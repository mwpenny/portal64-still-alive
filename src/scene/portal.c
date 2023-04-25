#include "portal.h"

#include "models/models.h"
#include "../graphics/screen_clipper.h"
#include "../graphics/graphics.h"
#include "../defs.h"
#include "dynamic_scene.h"
#include "../physics/collision_scene.h"
#include "../math/mathf.h"
#include "../math/vector2s16.h"
#include "../util/time.h"
#include "../levels/levels.h"
#include "./portal_surface_generator.h"

#include "../build/assets/models/portal/portal_blue.h"
#include "../build/assets/models/portal/portal_blue_filled.h"
#include "../build/assets/models/portal/portal_blue_face.h"
#include "../build/assets/models/portal/portal_orange.h"
#include "../build/assets/models/portal/portal_orange_face.h"
#include "../build/assets/models/portal/portal_orange_filled.h"


#define CALC_SCREEN_SPACE(clip_space, screen_size) ((clip_space + 1.0f) * ((screen_size) / 2))

#define PORTAL_COVER_HEIGHT 0.708084f
#define PORTAL_COVER_WIDTH  0.84085f

struct Vector3 gPortalOutline[PORTAL_LOOP_SIZE] = {
    {0.0f, 1.0f * SCENE_SCALE * PORTAL_COVER_HEIGHT, 0},
    {0.353553f * SCENE_SCALE * PORTAL_COVER_WIDTH, 0.707107f * SCENE_SCALE * PORTAL_COVER_HEIGHT, 0},
    {0.5f * SCENE_SCALE * PORTAL_COVER_WIDTH, 0.0f, 0},
    {0.353553f * SCENE_SCALE * PORTAL_COVER_WIDTH, -0.707107f * SCENE_SCALE * PORTAL_COVER_HEIGHT, 0},
    {0.0f, -1.0f * SCENE_SCALE * PORTAL_COVER_HEIGHT, 0},
    {-0.353553f * SCENE_SCALE * PORTAL_COVER_WIDTH, -0.707107f * SCENE_SCALE * PORTAL_COVER_HEIGHT, 0},
    {-0.5f * SCENE_SCALE * PORTAL_COVER_WIDTH, 0.0f, 0},
    {-0.353553f * SCENE_SCALE * PORTAL_COVER_WIDTH, 0.707107f * SCENE_SCALE * PORTAL_COVER_HEIGHT, 0},
};

#define PORTAL_CLIPPING_PLANE_BIAS  (SCENE_SCALE * 0.25f)

#define PORTAL_OPACITY_FADE_TIME    0.6f
#define PORTAL_GROW_TIME            0.3f

void portalInit(struct Portal* portal, enum PortalFlags flags) {
    transformInitIdentity(&portal->transform);
    portal->flags = flags;
    portal->opacity = 1.0f;
    portal->scale = 0.0f;
    portal->portalSurfaceIndex = -1;
    portal->transformIndex = NO_TRANSFORM_INDEX;
}

void portalUpdate(struct Portal* portal, int isOpen) {
    if (isOpen && portal->opacity > 0.0f) {
        portal->opacity -= FIXED_DELTA_TIME * (1.0f / PORTAL_OPACITY_FADE_TIME);

        if (portal->opacity < 0.0f) {
            portal->opacity = 0.0f;
        }
    } else if (!isOpen) {
        portal->opacity = 1.0f;
    }

    if (portal->scale < 1.0f) {
        portal->scale += FIXED_DELTA_TIME * (1.0f / PORTAL_GROW_TIME);

        if (portal->scale > 1.0f) {
            portal->scale = 1.0f;
        }
        portal->flags |= PortalFlagsNeedsNewHole;
        
    }
}

int portalAttachToSurface(struct Portal* portal, struct PortalSurface* surface, int surfaceIndex, struct Transform* portalAt, int just_checking) {
    // determine if portal is on surface
    if (!portalSurfaceIsInside(surface, portalAt)) {
        return 0;
    }
    if (just_checking){
        return 1;
    }
    // find all portal edge intersections
    struct Vector2s16 correctPosition;
    struct Vector2s16 portalOutline[PORTAL_LOOP_SIZE];
    if (!portalSurfaceAdjustPosition(surface, portalAt, &correctPosition, portalOutline)) {
        return 0;
    }
    

    for (int i = 0; i < PORTAL_LOOP_SIZE; ++i) {
        vector2s16Sub(&portalOutline[i], &correctPosition, &portal->originCentertedLoop[i]);
    }

    portal->flags |= PortalFlagsNeedsNewHole;
    portal->fullSizeLoopCenter = correctPosition;
    portal->portalSurfaceIndex = surfaceIndex;
    
    portalSurfaceInverse(surface, &correctPosition, &portalAt->position);

    return 1;
}

int portalSurfaceCutNewHole(struct Portal* portal, int portalIndex) {
    portal->flags &= ~PortalFlagsNeedsNewHole;

    if (portal->portalSurfaceIndex == -1) {
        return 1;
    }

    struct Vector2s16 scaledLoop[PORTAL_LOOP_SIZE];

    int fixedPointScale = (int)(0x10000 * portal->scale);

    for (int i = 0; i < PORTAL_LOOP_SIZE; ++i) {
        scaledLoop[i].x = ((portal->originCentertedLoop[i].x * fixedPointScale) >> 16) + portal->fullSizeLoopCenter.x;
        scaledLoop[i].y = ((portal->originCentertedLoop[i].y * fixedPointScale) >> 16) + portal->fullSizeLoopCenter.y;
    }

    struct PortalSurface* currentSurface = &gCurrentLevel->portalSurfaces[portal->portalSurfaceIndex];

    struct PortalSurface newSurface;

    if (!portalSurfacePokeHole(currentSurface, scaledLoop, &newSurface)) {
        return 0;
    }
    
    portalSurfaceReplace(portal->portalSurfaceIndex, portal->roomIndex, portalIndex, &newSurface);

    return 1;
}

void portalCheckForHoles(struct Portal* portals) {

    //prevents cutting two holes at the same time on top of eachother (causes crash)
    if (portalSurfaceAreBothOnSameSurface() && ((portals[0].scale < 1.0f) || (portals[1].scale < 1.0f))){
        return;
    }

    if (((portals[1].flags & PortalFlagsNeedsNewHole) != 0)) {
        if (portalSurfaceShouldSwapOrder(1)){
            portalSurfacePreSwap(1);
            portalSurfaceCutNewHole(&portals[0], 0);
        }
        else{
            portalSurfaceRevert(1);
            portalSurfaceCutNewHole(&portals[1], 1);
        }
        return;
    }

    if (((portals[0].flags & PortalFlagsNeedsNewHole) != 0)){
        if (portalSurfaceShouldSwapOrder(0)){
            portalSurfacePreSwap(0);
            portalSurfaceCutNewHole(&portals[1], 1);   
        }
        else{
            portalSurfaceRevert(0);
            portalSurfaceCutNewHole(&portals[0], 0);
        }
        return;
    }

}

int minkowsiSumAgainstPortal(void* data, struct Vector3* direction, struct Vector3* output) {
    struct Transform* transform = (struct Transform*)data;
    struct Vector3 localDir;
    struct Quaternion inverseRotation;

    quatConjugate(&transform->rotation, &inverseRotation);
    quatMultVector(&inverseRotation, direction, &localDir);

    float maxDot = vector3Dot(&gPortalOutline[0], &localDir);
    int maxDotIndex = 0;

    for (int i = 0; i < 9; ++i) {
        if (maxDot < 0.0f) {
            maxDotIndex = (maxDotIndex + 4) & 0x7;
            maxDot = vector3Dot(&gPortalOutline[maxDotIndex], &localDir);
        } else {
            int prevIndex = (i - 1) & 0x7;
            int nextIndex = (i + 1) & 0x7;

            float prevDot = vector3Dot(&gPortalOutline[prevIndex], &localDir);
            float nextDot = vector3Dot(&gPortalOutline[nextIndex], &localDir);

            if (prevDot > maxDot) {
                maxDotIndex = prevIndex;
                maxDot = prevDot;
            } else if (nextDot > maxDot) {
                maxDotIndex = nextIndex;
                maxDot = nextDot;
            } else {
                break;
            }
        }
    }

    quatMultVector(&transform->rotation, &gPortalOutline[maxDotIndex], output);
    vector3Scale(output, output, 1.2f / SCENE_SCALE);
    vector3Add(output, &transform->position, output);

    return 1 << maxDotIndex;
}