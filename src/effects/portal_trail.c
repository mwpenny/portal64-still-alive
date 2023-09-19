#include "portal_trail.h"

#include "../math/vector2.h"
#include "../math/mathf.h"
#include "../defs.h"

#include "../build/assets/models/portal_gun/ball_trail.h"
#include "../build/assets/materials/static.h"
#include "../graphics/color.h"

#include "../util/time.h"

#define TRAIL_LENGTH    8.0f
#define FADE_IN_LENGTH  4.0f
#define SEGMENT_LENGTH  2.0f
#define SEGMENT_ROTATION    (152 * M_PI / 180.0f)

struct Transform gTrailSectionOffset = {
    {0.0f, 0.0f, -SEGMENT_LENGTH},
    {0.0f, 0.0f, 0.0f, 1.0f},
    {1.0f, 1.0f, 1.0f},
};

void portalTrailInit(struct PortalTrail* trail) {
    quatAxisAngle(&gForward, SEGMENT_ROTATION, &gTrailSectionOffset.rotation);
    transformToMatrixL(&gTrailSectionOffset, &trail->sectionOffset, SCENE_SCALE);
    guMtxIdent(&trail->baseTransform[0]);
    guMtxIdent(&trail->baseTransform[1]);

    trail->currentBaseTransform = 0;
    trail->lastDistance = 0.0f;
    trail->maxDistance = -TRAIL_LENGTH;
}

void portalTrailUpdateBaseTransform(struct PortalTrail* trail) {
    trail->currentBaseTransform ^= 1;
    transformToMatrixL(&trail->trailTransform, &trail->baseTransform[trail->currentBaseTransform], SCENE_SCALE);
    osWritebackDCache(&trail->baseTransform[trail->currentBaseTransform], sizeof(Mtx));
}

void portalTrailPlay(struct PortalTrail* trail, struct Vector3* from, struct Vector3* to) {
    trail->trailTransform.position = *from;
    struct Vector3 dir;
    vector3Sub(to, from, &dir);
    struct Vector3 randomUp;
    randomUp.x = randomInRangef(-1.0f, 1.0f);
    randomUp.y = randomInRangef(-1.0f, 1.0f);
    randomUp.z = randomInRangef(-1.0f, 1.0f);
    quatLook(&dir, &randomUp, &trail->trailTransform.rotation);
    vector3Normalize(&dir, &trail->direction);
    trail->trailTransform.scale = gOneVec;

    portalTrailUpdateBaseTransform(trail);

    trail->lastDistance = 0.0f;
    trail->maxDistance = vector3Dot(&dir, &trail->direction);

    if (trail->maxDistance < SEGMENT_LENGTH) {
        trail->maxDistance = -TRAIL_LENGTH;
    }
}

void portalTrailUpdate(struct PortalTrail* trail) {
    if (trail->lastDistance >= trail->maxDistance + TRAIL_LENGTH) {
        return;
    }

    trail->lastDistance += FIXED_DELTA_TIME * PORTAL_PROJECTILE_SPEED;

    if (trail->lastDistance > TRAIL_LENGTH + SEGMENT_LENGTH) {
        trail->lastDistance -= SEGMENT_LENGTH;
        trail->maxDistance -= SEGMENT_LENGTH;
        
        struct Transform tmp;
        transformConcat(&trail->trailTransform, &gTrailSectionOffset, &tmp);
        trail->trailTransform = tmp;
        portalTrailUpdateBaseTransform(trail);
    }
}

struct Coloru8 gTrailColor[] = {
    {200, 100, 50, 255},
    {50, 70, 200, 255},
};

void portalTrailRender(struct PortalTrail* trail, struct RenderState* renderState, struct MaterialState* materialState, struct Camera* fromCamera, int portalIndex) {
    if (trail->lastDistance >= trail->maxDistance + TRAIL_LENGTH) {
        return;
    }

    materialStateSet(materialState, PORTAL_TRAIL_INDEX, renderState);

    struct Coloru8* color = &gTrailColor[portalIndex];

    struct Ray cameraRay;
    cameraRay.origin = fromCamera->transform.position;
    quatMultVector(&fromCamera->transform.rotation, &gForward, &cameraRay.dir);
    vector3Negate(&cameraRay.dir, &cameraRay.dir);

    struct Vector3 pointAlongTrail;
    vector3AddScaled(&trail->trailTransform.position, &trail->direction, trail->lastDistance - TRAIL_LENGTH, &pointAlongTrail);
    int minDistance = fogIntValue(cameraClipDistance(fromCamera, rayDetermineDistance(&cameraRay, &pointAlongTrail)));

    vector3AddScaled(&trail->trailTransform.position, &trail->direction, trail->lastDistance, &pointAlongTrail);
    int maxDistance = fogIntValue(cameraClipDistance(fromCamera, rayDetermineDistance(&cameraRay, &pointAlongTrail)));

    if (maxDistance <= minDistance) {
        maxDistance = minDistance + 1;

        if (maxDistance > 1000) {
            maxDistance = 1000;
            minDistance = 999;
        }
    }

    float currentDistance = trail->lastDistance - TRAIL_LENGTH;

    int alpha = 0;

    if (currentDistance < -(TRAIL_LENGTH - FADE_IN_LENGTH)) {
        alpha = (int)(((TRAIL_LENGTH - FADE_IN_LENGTH) + currentDistance) * (-255.0f / TRAIL_LENGTH));

        if (alpha < 0) {
            alpha = 0;
        }

        if (alpha > 255) {
            alpha = 255;
        }
    }

    gSPFogPosition(renderState->dl++, minDistance, maxDistance);
    gDPSetPrimColor(renderState->dl++, 255, 255, color->r, color->g, color->b, alpha);
    gSPMatrix(renderState->dl++, &trail->baseTransform[trail->currentBaseTransform], G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL);

    int hasMore = 1;

    while (hasMore) {
        currentDistance += SEGMENT_LENGTH;

        hasMore = currentDistance < trail->lastDistance && currentDistance < trail->maxDistance;

        if (currentDistance <= 0.0f) {
            continue;
        }

        gSPDisplayList(renderState->dl++, portal_gun_ball_trail_model_gfx);

        if (hasMore) {
            gSPMatrix(renderState->dl++, &trail->sectionOffset, G_MTX_MODELVIEW | G_MTX_NOPUSH | G_MTX_MUL);
        }
    }

    gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
}