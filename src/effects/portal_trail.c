#include "portal_trail.h"

#include "graphics/color.h"
#include "math/vector2.h"
#include "math/math.h"
#include "math/ray.h"
#include "util/frame_time.h"

#include "codegen/assets/models/portal_gun/ball_trail.h"
#include "codegen/assets/materials/static.h"

#define PORTAL_TRAIL_LENGTH             8.0f
#define PORTAL_TRAIL_SEGMENT_LENGTH     2.0f
#define PORTAL_TRAIL_SEGMENT_ROTATION   (152 * M_PI / 180.0f)
#define PORTAL_TRAIL_PERPENDICULAR_DOT  0.0875f  // acos(0.0875) = ~5 degrees

#define PORTAL_TRAIL_FADE_IN_DISTANCE   4.0f
#define PORTAL_TRAIL_MAX_FADE_IN        127

static struct Transform sTrailSegmentOffset = {
    {0.0f, 0.0f, -PORTAL_TRAIL_SEGMENT_LENGTH},
    {0.0f, 0.0f, 0.0f, 1.0f},
    {1.0f, 1.0f, 1.0f},
};

static struct Coloru8 sTrailColor[] = {
    {200, 100, 50, 255},
    {50, 70, 200, 255},
};

void portalTrailInit(struct PortalTrail* trail) {
    quatAxisAngle(&gForward, PORTAL_TRAIL_SEGMENT_ROTATION, &sTrailSegmentOffset.rotation);
    transformToMatrixL(&sTrailSegmentOffset, &trail->segmentOffset, SCENE_SCALE);

    guMtxIdent(&trail->baseTransform[0]);
    guMtxIdent(&trail->baseTransform[1]);
    trail->currentBaseTransform = 0;

    trail->headDistance = 0.0f;
    trail->targetDistance = -PORTAL_TRAIL_LENGTH;
}

static void portalTrailUpdateBaseTransform(struct PortalTrail* trail) {
    // Double buffered to avoid corrupting in-progress frames
    trail->currentBaseTransform ^= 1;
    transformToMatrixL(&trail->transform, &trail->baseTransform[trail->currentBaseTransform], SCENE_SCALE);
    osWritebackDCache(&trail->baseTransform[trail->currentBaseTransform], sizeof(Mtx));
}

void portalTrailPlay(struct PortalTrail* trail, struct Vector3* from, struct Vector3* to) {
    struct Vector3 dir;
    vector3Sub(to, from, &dir);

    float distSquared = vector3MagSqrd(&dir);
    if (distSquared < (PORTAL_TRAIL_SEGMENT_LENGTH * PORTAL_TRAIL_SEGMENT_LENGTH)) {
        // Don't show trail for short distances
        return;
    }

    struct Vector3 randomUp;
    vector3Random(&randomUp);
    quatLook(&dir, &randomUp, &trail->transform.rotation);

    trail->transform.position = *from;
    trail->transform.scale = gOneVec;
    portalTrailUpdateBaseTransform(trail);

    trail->headDistance = 0.0f;
    trail->targetDistance = sqrtf(distSquared);
    vector3Scale(&dir, &trail->direction, 1.0f / trail->targetDistance);
}

void portalTrailUpdate(struct PortalTrail* trail) {
    float tailDistance = trail->headDistance - PORTAL_TRAIL_LENGTH;
    if (tailDistance >= trail->targetDistance) {
        return;
    }

    trail->headDistance += PORTAL_TRAIL_SPEED * FIXED_DELTA_TIME;

    if (trail->headDistance > PORTAL_TRAIL_LENGTH + PORTAL_TRAIL_SEGMENT_LENGTH) {
        // Keep the trail at the proper length. Moving up in one segment chunks
        // rather than continuously makes it appear to hang in the air and
        // gradually disappear (less like a projectile).
        trail->headDistance -= PORTAL_TRAIL_SEGMENT_LENGTH;
        trail->targetDistance -= PORTAL_TRAIL_SEGMENT_LENGTH;

        struct Transform tmp;
        transformConcat(&trail->transform, &sTrailSegmentOffset, &tmp);
        trail->transform = tmp;
        portalTrailUpdateBaseTransform(trail);
    }
}

void portalTrailRender(
    struct PortalTrail* trail,
    struct RenderState* renderState,
    struct MaterialState* materialState,
    struct Camera* fromCamera,
    int portalIndex
) {
    float tailDistance = trail->headDistance - PORTAL_TRAIL_LENGTH;
    if (tailDistance >= trail->targetDistance) {
        return;
    }

    materialStateSet(materialState, PORTAL_TRAIL_INDEX, renderState);

    // Use fog to hide/reveal trail based on distance from camera
    struct Ray cameraRay;
    cameraRay.origin = fromCamera->transform.position;
    quatMultVector(&fromCamera->transform.rotation, &gForward, &cameraRay.dir);
    vector3Negate(&cameraRay.dir, &cameraRay.dir);

    if (fabsf(vector3Dot(&cameraRay.dir, &trail->direction)) >= PORTAL_TRAIL_PERPENDICULAR_DOT) {
        struct Vector3 pointAlongTrail;
        vector3AddScaled(&trail->transform.position, &trail->direction, tailDistance, &pointAlongTrail);

        float d = rayDetermineDistance(&cameraRay, &pointAlongTrail);
        int minDistance = (d >= 0.0f) ? fogIntValue(cameraClipDistance(fromCamera, d)) : 0;

        vector3AddScaled(&trail->transform.position, &trail->direction, trail->headDistance, &pointAlongTrail);
        d = rayDetermineDistance(&cameraRay, &pointAlongTrail);
        int maxDistance = (d >= 0.0f) ? fogIntValue(cameraClipDistance(fromCamera, d)) : 0;

        if (minDistance == maxDistance) {
            // Close to near/far plane. Hide it.
            minDistance = 995;
            maxDistance = 1000;
        }

        gSPFogPosition(renderState->dl++, minDistance, maxDistance);
    } else {
        // Disable fog when near-perpendicular to ensure visibility. Lack of
        // fade is better than no trail. Setting vertex alpha manually is more
        // robust, but overkill. The trail is fast and usually seen straight-on.
        gSPClearGeometryMode(renderState->dl++, G_FOG);
    }

    // Fade at start
    int alpha = 0;
    if (trail->headDistance < PORTAL_TRAIL_FADE_IN_DISTANCE) {
        float fadeAmount = 1.0f - (trail->headDistance * (1.0f / PORTAL_TRAIL_FADE_IN_DISTANCE));
        alpha = PORTAL_TRAIL_MAX_FADE_IN * fadeAmount;
    }

    struct Coloru8* color = &sTrailColor[portalIndex];
    gDPSetPrimColor(renderState->dl++, 255, 255, color->r, color->g, color->b, alpha);

    // Render segments
    gSPMatrix(renderState->dl++, &trail->baseTransform[trail->currentBaseTransform], G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL);

    // The trail moves into its target so segments can disappear nicely, but
    // they should not continue on the other side
    float maxSegmentDist = MIN(trail->headDistance, trail->targetDistance);
    for (float d = 0.0f; d < maxSegmentDist; d += PORTAL_TRAIL_SEGMENT_LENGTH) {
        if (d > 0.0f) {
            gSPMatrix(renderState->dl++, &trail->segmentOffset, G_MTX_MODELVIEW | G_MTX_NOPUSH | G_MTX_MUL);
        }

        gSPDisplayList(renderState->dl++, portal_gun_ball_trail_model_gfx);
    }

    gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
}
