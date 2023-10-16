#include "portal.h"

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
#include "../scene/dynamic_scene.h"

#include "../build/assets/models/portal/portal_blue.h"
#include "../build/assets/models/portal/portal_blue_filled.h"
#include "../build/assets/models/portal/portal_blue_face.h"
#include "../build/assets/models/portal/portal_collider.h"
#include "../build/assets/models/portal/portal_orange.h"
#include "../build/assets/models/portal/portal_orange_face.h"
#include "../build/assets/models/portal/portal_orange_filled.h"

struct ColliderTypeData gPortalColliderType = {
    CollisionShapeTypeMesh,
    &portal_portal_collider_collider,
    0.0f, 0.6f,
    &gMeshColliderCallbacks
};

#define CALC_SCREEN_SPACE(clip_space, screen_size) ((clip_space + 1.0f) * ((screen_size) / 2))

struct Vector3 gPortalOutline[PORTAL_LOOP_SIZE] = {
    {0.0f, 1.0f * SCENE_SCALE * PORTAL_COVER_HEIGHT_RADIUS, 0},
    {0.707107f * SCENE_SCALE * PORTAL_COVER_WIDTH_RADIUS, SCENE_SCALE * PORTAL_COVER_HEIGHT_RADIUS, 0},
    {1.0f * SCENE_SCALE * PORTAL_COVER_WIDTH_RADIUS, 0.0f, 0},
    {0.707107f * SCENE_SCALE * PORTAL_COVER_WIDTH_RADIUS, -1.0f * SCENE_SCALE * PORTAL_COVER_HEIGHT_RADIUS, 0},
    {0.0f, -1.0f * SCENE_SCALE * PORTAL_COVER_HEIGHT_RADIUS, 0},
    {-0.707107f * SCENE_SCALE * PORTAL_COVER_WIDTH_RADIUS, -1.0f * SCENE_SCALE * PORTAL_COVER_HEIGHT_RADIUS, 0},
    {-1.0f * SCENE_SCALE * PORTAL_COVER_WIDTH_RADIUS, 0.0f, 0},
    {-0.707107f * SCENE_SCALE * PORTAL_COVER_WIDTH_RADIUS, SCENE_SCALE * PORTAL_COVER_HEIGHT_RADIUS, 0},
};

#define PORTAL_CLIPPING_PLANE_BIAS  (SCENE_SCALE * 0.25f)

#define PORTAL_OPACITY_FADE_TIME    0.6f
#define PORTAL_GROW_TIME            0.3f

void portalInit(struct Portal* portal, enum PortalFlags flags) {
    collisionObjectInit(&portal->collisionObject, &gPortalColliderType, &portal->rigidBody, 1.0f, COLLISION_LAYERS_STATIC | COLLISION_LAYERS_TANGIBLE);
    rigidBodyMarkKinematic(&portal->rigidBody);
    portal->flags = flags;
    portal->opacity = 1.0f;
    portal->scale = 1.0f;
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
    }
}

void portalCalculateBB(struct Transform* portalTransform, struct Box3D* bb) {
    struct Vector3 portalUp;
    quatMultVector(&portalTransform->rotation, &gUp, &portalUp);
    struct Vector3 portalRight;
    quatMultVector(&portalTransform->rotation, &gRight, &portalRight);
    struct Vector3 portalNormal;
    vector3Cross(&portalUp, &portalRight, &portalNormal);

    bb->min = bb->max = portalTransform->position;

    struct Vector3 nextDir;
    vector3Scale(&portalUp, &nextDir, PORTAL_COVER_HEIGHT_RADIUS);
    box3DExtendDirection(bb, &nextDir, bb);
    vector3Negate(&nextDir, &nextDir);
    box3DExtendDirection(bb, &nextDir, bb);

    vector3Scale(&portalRight, &nextDir, PORTAL_COVER_WIDTH_RADIUS);
    box3DExtendDirection(bb, &nextDir, bb);
    vector3Negate(&nextDir, &nextDir);
    box3DExtendDirection(bb, &nextDir, bb);

    vector3Scale(&portalNormal, &nextDir, 0.1f);
    box3DExtendDirection(bb, &nextDir, bb);
    vector3Negate(&nextDir, &nextDir);
    box3DExtendDirection(bb, &nextDir, bb);
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
    
    portal->fullSizeLoopCenter = correctPosition;

    if (portal->portalSurfaceIndex == -1) {
        collisionSceneAddDynamicObject(&portal->collisionObject);
    }    

    portal->portalSurfaceIndex = surfaceIndex;
    
    portalSurfaceInverse(surface, &correctPosition, &portalAt->position);

    return 1;
}

int portalSurfaceCutNewHole(struct Portal* portal, int portalIndex) {
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
    
    portalSurfaceReplace(portal->portalSurfaceIndex, portal->rigidBody.currentRoom, portalIndex, portal->scale, &newSurface);

    return 1;
}

int portalNeedsToRemoveSecondPortal(struct Portal* portals, int shouldMoveFirstPortal) {
    int secondPortalSurfaceIndex = portalSurfaceGetSurfaceIndex(1);

    // second portal isn't placed so no need to temporarily remove it
    if (secondPortalSurfaceIndex == -1) {
        return 0;
    }

    // first portal doesn't need to move, so no need to temporarily remove
    // second portal
    if (!shouldMoveFirstPortal) {
        return 0;
    }

    // only remove second portal if the first one is moving to or from the same surface as the second
    return portals[0].portalSurfaceIndex == secondPortalSurfaceIndex || portalSurfaceGetSurfaceIndex(0) == secondPortalSurfaceIndex;
}

void portalCheckForHoles(struct Portal* portals) {
    int shouldMoveSecondPortal = portalSurfaceShouldMove(1, portals[1].portalSurfaceIndex, portals[1].scale);
    int shouldMoveFirstPortal = portalSurfaceShouldMove(0, portals[0].portalSurfaceIndex, portals[0].scale);

    if (shouldMoveSecondPortal || portalNeedsToRemoveSecondPortal(portals, shouldMoveFirstPortal)) {
        portalSurfaceRevert(1);
        shouldMoveSecondPortal = 1;
    }

    if (shouldMoveFirstPortal) {
        portalSurfaceRevert(0);
        portalSurfaceCutNewHole(&portals[0], 0);
    }

    if (shouldMoveSecondPortal) {
        portalSurfaceCutNewHole(&portals[1], 1);
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