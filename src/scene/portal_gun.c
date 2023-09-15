#include "portal_gun.h"

#include "../physics/collision_scene.h"
#include "../physics/collision_cylinder.h"

#include "../../build/assets/models/grav_flare.h"
#include "../../build/assets/models/portal_gun/v_portalgun.h"
#include "../../build/assets/materials/static.h"

#define PORTAL_GUN_RECOIL_TIME (0.18f)

struct Vector2 gGunColliderEdgeVectors[] = {
    {0.0f, 1.0f},
    {0.707f, 0.707f},
    {1.0f, 0.0f},
    {0.707f, -0.707f},
};

struct CollisionQuad gGunColliderFaces[8];

struct CollisionCylinder gGunCollider = {
    0.05f,
    0.1f,
    gGunColliderEdgeVectors,
    sizeof(gGunColliderEdgeVectors) / sizeof(*gGunColliderEdgeVectors),
    gGunColliderFaces,
};

struct ColliderTypeData gGunColliderData = {
    CollisionShapeTypeCylinder,
    &gGunCollider,
    0.0f,
    0.6f,
    &gCollisionCylinderCallbacks,
};

void portalGunInit(struct PortalGun* portalGun, struct Transform* at){
    collisionObjectInit(&portalGun->collisionObject, &gGunColliderData, &portalGun->rigidBody, 1.0f, 0);
    collisionSceneAddDynamicObject(&portalGun->collisionObject);
    portalGun->rigidBody.transform = *at;
    portalGun->rigidBody.transform.scale = gOneVec;
    portalGun->rigidBody.currentRoom = 0;
    portalGun->rigidBody.velocity = gZeroVec;
    portalGun->rigidBody.angularVelocity = gZeroVec;
    portalGun->portalGunVisible = 0;
    portalGun->shootAnimationTimer = 0.0;

    portalGun->projectiles[0].dynamicId = -1;
    portalGun->projectiles[0].roomIndex = -1;
    portalGun->projectiles[1].dynamicId = -1;
    portalGun->projectiles[1].roomIndex = -1;
}

void portalGunRenderReal(struct PortalGun* portalGun, struct RenderState* renderState){
    portalGun->rigidBody.transform.scale = gOneVec;
    Mtx* matrix = renderStateRequestMatrices(renderState, 1);

    if (!matrix) {
        return;
    }

    transformToMatrixL(&portalGun->rigidBody.transform, matrix, SCENE_SCALE);
    gSPMatrix(renderState->dl++, matrix, G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL);
    gSPDisplayList(renderState->dl++, portal_gun_v_portalgun_model_gfx);
    gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
}

#define PORTAL_PROJECTILE_RADIUS    0.2f
#define NO_HIT_DISTANCE             20.0f
#define PORTAL_PROJECTILE_SPEED     10.0f
#define MAX_PROJECTILE_DISTANCE     100.0f

void portalGunUpdate(struct PortalGun* portalGun, struct Player* player) {
    if (player->flags & (PlayerHasFirstPortalGun | PlayerHasSecondPortalGun)) {
        portalGun->portalGunVisible = 1;
    } else {
        portalGun->portalGunVisible = 0;
    }

    if (player->flags & PlayerJustShotPortalGun && portalGun->shootAnimationTimer <= 0.0f) {
        portalGun->shootAnimationTimer = PORTAL_GUN_RECOIL_TIME;
    }

    if (portalGun->shootAnimationTimer >= 0.0f) {
        portalGun->shootAnimationTimer -= FIXED_DELTA_TIME;
        if (portalGun->shootAnimationTimer <= 0.0f){
            portalGun->shootAnimationTimer = 0.0f;
            player->flags &= ~PlayerJustShotPortalGun;
        }
    }

    for (int i = 0; i < 2; ++i) {
        struct PortalGunProjectile* projectile = &portalGun->projectiles[i];

        if (projectile->dynamicId == -1) {
            continue;
        }

        struct RaycastHit hit;

        if (collisionSceneRaycast(&gCollisionScene, projectile->roomIndex, &projectile->positionDirection, COLLISION_LAYERS_STATIC | COLLISION_LAYERS_BLOCK_PORTAL, PORTAL_PROJECTILE_SPEED * FIXED_DELTA_TIME + 0.1f, 0, &hit)) {
            // TODO open portal
            dynamicSceneRemove(projectile->dynamicId);
            projectile->dynamicId = -1;
        } else {
            projectile->roomIndex = hit.roomIndex;
        }

        vector3AddScaled(
            &projectile->positionDirection.origin, 
            &projectile->positionDirection.dir, 
            PORTAL_PROJECTILE_SPEED * FIXED_DELTA_TIME, 
            &projectile->positionDirection.origin
        );
        projectile->distance += PORTAL_PROJECTILE_SPEED * FIXED_DELTA_TIME;
    }
}

void portalBallRender(void* data, struct RenderScene* renderScene, struct Transform* fromView) {
    struct PortalGunProjectile* projectile = (struct PortalGunProjectile*)data;
    struct Transform transform;

    if (projectile->distance < projectile->maxDistance) {
        vector3AddScaled(
            &projectile->positionDirection.origin, 
            &projectile->effectOffset, 
            1.0f - projectile->distance / projectile->maxDistance, 
            &transform.position
        );
    } else {
        transform.position = projectile->positionDirection.origin;
    }

    transform.rotation = fromView->rotation;
    vector3Scale(&gOneVec, &transform.scale, PORTAL_PROJECTILE_RADIUS);

    Mtx* mtx = renderStateRequestMatrices(renderScene->renderState, 1);

    transformToMatrixL(&transform, mtx, SCENE_SCALE);

    renderSceneAdd(renderScene, grav_flare_model_gfx, mtx, GRAV_FLARE_INDEX, &transform.position, NULL);
}

struct Vector3 gPortalGunExit = {0.0f, 0.0f, 0.154008};

void portalGunFire(struct PortalGun* portalGun, int portalIndex, struct Ray* ray, struct Vector3* playerUp, int roomIndex) {
    struct PortalGunProjectile* projectile = &portalGun->projectiles[portalIndex];

    struct RaycastHit hit;

    if (!collisionSceneRaycast(&gCollisionScene, roomIndex, ray, COLLISION_LAYERS_STATIC | COLLISION_LAYERS_BLOCK_PORTAL, 1000000.0f, 0, &hit)) {
        vector3AddScaled(&ray->origin, &ray->dir, NO_HIT_DISTANCE, &hit.at);
        hit.distance = NO_HIT_DISTANCE;
        hit.normal = gZeroVec;
        hit.object = NULL;
        hit.roomIndex = roomIndex;
        hit.throughPortal = NULL;
    }
    
    projectile->positionDirection = *ray;
    projectile->roomIndex = roomIndex;
    projectile->playerUp = *playerUp;

    projectile->distance = 0.0f;
    projectile->maxDistance = hit.distance;

    struct Vector3 fireFrom;
    transformPoint(&portalGun->rigidBody.transform, &gPortalGunExit, &fireFrom);
    vector3Sub(&fireFrom, &ray->origin, &projectile->effectOffset);

    if (projectile->dynamicId == -1) {
        projectile->dynamicId = dynamicSceneAddViewDependant(projectile, portalBallRender, &fireFrom, 0.5f);
    }
}