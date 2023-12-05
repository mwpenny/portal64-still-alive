#include "portal_gun.h"

#include "../physics/collision_scene.h"
#include "../physics/collision_cylinder.h"
#include "./scene.h"

#include "../levels/material_state.h"

#include "../effects/effect_definitions.h"
#include "./render_plan.h"

#include "../../build/assets/models/grav_flare.h"
#include "../../build/assets/models/portal_gun/v_portalgun.h"
#include "../../build/assets/materials/static.h"

// the portal gun is rendered with a different field of view than the scene
#define FIRST_PERSON_POV_FOV    42.45f

#define PORTAL_GUN_RECOIL_TIME (0.22f)

#define PORTAL_GUN_NEAR_PLANE   0.05f

#define PORTAL_GUN_MOI          0.00395833375f

#define PORTAL_GUN_SCALE        512.0f

struct Quaternion gFlipAroundY = {0.0f, 1.0f, 0.0f, 0.0f};

struct Transform gGunTransform = {
    {0.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f, 0.0f},
    {1.0f, 1.0f, 1.0f},
};

void portalGunInit(struct PortalGun* portalGun, struct Transform* at, int isFreshStart){
    skArmatureInit(&portalGun->armature, &portal_gun_v_portalgun_armature);
    skAnimatorInit(&portalGun->animator, portal_gun_v_portalgun_armature.numberOfBones);
    portalGun->portalGunVisible = 0;
    portalGun->shootAnimationTimer = 0.0;
    portalGun->shootTotalAnimationTimer = 0.0;

    portalGun->projectiles[0].roomIndex = -1;
    portalGun->projectiles[1].roomIndex = -1;

    portalTrailInit(&portalGun->projectiles[0].trail);
    portalTrailInit(&portalGun->projectiles[1].trail);

    portalGun->rotation = at->rotation;

    if (isFreshStart) {
        skAnimatorRunClip(&portalGun->animator, &portal_gun_v_portalgun_Armature_draw_clip, 0.0f, 0);
        // the first time the scene renders, the animation clip hasn't started yet
        // this just hides the gun offscreen so it doesn't show up for a single frame
        portalGun->armature.pose[0].position.y = -1.0f;
    } else {
        skAnimatorRunClip(&portalGun->animator, &portal_gun_v_portalgun_Armature_idle_clip, 0.0f, 0);
    } 
}

#define PORTAL_PROJECTILE_RADIUS    0.15f

#define DISTANCE_FADE_SCALAR        (255.0f / 5.0f)

struct Coloru8 gProjectileColor[] = {
    {200, 100, 50, 255},
    {50, 70, 200, 255},
};

void portalBallRender(struct PortalGunProjectile* projectile, struct RenderState* renderState, struct MaterialState* materialState, struct Transform* fromView, int portalIndex) {
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

    Mtx* mtx = renderStateRequestMatrices(renderState, 1);

    transformToMatrixL(&transform, mtx, SCENE_SCALE);

    struct Coloru8* color = &gProjectileColor[portalIndex];

    float alpha = projectile->distance * DISTANCE_FADE_SCALAR;

    if (alpha > 255.0f) {
        alpha = 255.0f;
    }


    if (projectile->distance == 0.0f) {
        materialStateSet(materialState, PORTAL_2_PARTICLE_INDEX, renderState);
    } else {
        materialStateSet(materialState, BRIGHTGLOW_Y_INDEX, renderState);
        gDPSetPrimColor(renderState->dl++, 255, 255, color->r, color->g, color->b, (u8)alpha);
    }

    gSPMatrix(renderState->dl++, mtx, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    gSPDisplayList(renderState->dl++, grav_flare_model_gfx);
    gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
}

extern LookAt gLookAt;
extern float getAspect();

void portalGunRenderReal(struct PortalGun* portalGun, struct RenderState* renderState, struct Camera* fromCamera, int lastFiredIndex) {
    struct MaterialState materialState;
    materialStateInit(&materialState, DEFAULT_INDEX);
    
    for (int i = 0; i < 2; ++i) {
        struct PortalGunProjectile* projectile = &portalGun->projectiles[i];

        portalTrailRender(&projectile->trail, renderState, &materialState, fromCamera, i);

        if (projectile->roomIndex == -1) {
            continue;
        }

        portalBallRender(projectile, renderState, &materialState, &fromCamera->transform, i);
    }

    if (!portalGun->portalGunVisible) {
        return;
    }

    Mtx* matrix = renderStateRequestMatrices(renderState, 2);

    if (!matrix) {
        return;
    }

    u16 perspectiveNormalize;
    guPerspective(&matrix[1], &perspectiveNormalize, FIRST_PERSON_POV_FOV, getAspect(), 0.05f * SCENE_SCALE, 4.0f * SCENE_SCALE, 1.0f);
    gSPMatrix(renderState->dl++, &matrix[1], G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);

    gSPLookAt(renderState->dl++, &gLookAt);
    gDPPipeSync(renderState->dl++);
    // set the portal indicator color
    if (lastFiredIndex >= 0 && lastFiredIndex <= 1) {
        struct Coloru8 color = gProjectileColor[lastFiredIndex];
        gDPSetEnvColor(renderState->dl++, color.r, color.g, color.b, 255);
    } else {
        gDPSetEnvColor(renderState->dl++, 128, 128, 128, 255);
    }

    struct Quaternion relativeRotation;
    struct Quaternion inverseCameraRotation;
    quatConjugate(&fromCamera->transform.rotation, &inverseCameraRotation);
    quatMultiply(&inverseCameraRotation, &portalGun->rotation, &relativeRotation);

    quatMultiply(&relativeRotation, &gFlipAroundY, &gGunTransform.rotation);
    
    LookAt* lookAt = renderStateRequestLookAt(renderState);
    *lookAt = gLookAt;

    struct Vector3 lookDirection;
    quatMultVector(&inverseCameraRotation, &gForward, &lookDirection);
    vector3ToVector3u8(&lookDirection, (struct Vector3u8*)&lookAt->l[0].l.dir);

    quatMultVector(&inverseCameraRotation, &gUp, &lookDirection);
    vector3ToVector3u8(&lookDirection, (struct Vector3u8*)&lookAt->l[1].l.dir);

    gSPLookAt(renderState->dl++, lookAt);
    
    transformToMatrixL(&gGunTransform, &matrix[0], PORTAL_GUN_SCALE);
    gSPMatrix(renderState->dl++, &matrix[0], G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL);
    skRenderObject(&portalGun->armature, NULL, renderState);
    gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);

    gSPDisplayList(renderState->dl++, static_default);
}

#define NO_HIT_DISTANCE             20.0f
#define MAX_PROJECTILE_DISTANCE     100.0f

void portalGunUpdatePosition(struct PortalGun* portalGun, struct Player* player) {
    if (player->passedThroughPortal) {
        int portalIndex = player->passedThroughPortal - 1;

        struct Transform* transform = collisionSceneTransformToPortal(portalIndex);

        if (transform) {
            struct Quaternion newRotation;
            quatMultiply(&transform->rotation, &portalGun->rotation, &newRotation);
            portalGun->rotation = newRotation;
        }
    }

    quatLerp(&portalGun->rotation, &player->lookTransform.rotation, 0.45f, &portalGun->rotation);
}

void portalGunUpdate(struct PortalGun* portalGun, struct Player* player) {
    skAnimatorUpdate(&portalGun->animator, portalGun->armature.pose, FIXED_DELTA_TIME);
    portalGunUpdatePosition(portalGun, player);

    if (player->flags & (PlayerHasFirstPortalGun | PlayerHasSecondPortalGun)) {
        portalGun->portalGunVisible = 1;
    } else {
        portalGun->portalGunVisible = 0;
    }

    if (player->flags & PlayerJustShotPortalGun && portalGun->shootAnimationTimer <= 0.0f) {
        portalGun->shootAnimationTimer = PORTAL_GUN_RECOIL_TIME;
        portalGun->shootTotalAnimationTimer = PORTAL_GUN_RECOIL_TIME * 2.0f; 
    }

    if (portalGun->shootAnimationTimer >= 0.0f) {
        portalGun->shootAnimationTimer -= FIXED_DELTA_TIME;
        if (portalGun->shootAnimationTimer <= 0.0f){
            portalGun->shootAnimationTimer = 0.0f;
            player->flags &= ~PlayerJustShotPortalGun;
        }
    }
    if (portalGun->shootTotalAnimationTimer >= 0.0f) {
        portalGun->shootTotalAnimationTimer -= FIXED_DELTA_TIME;
        if (portalGun->shootTotalAnimationTimer <= 0.0f){
            portalGun->shootTotalAnimationTimer = 0.0f;
        }
    }

    for (int i = 0; i < 2; ++i) {
        struct PortalGunProjectile* projectile = &portalGun->projectiles[i];

        portalTrailUpdate(&projectile->trail);

        if (projectile->roomIndex == -1) {
            continue;
        }

        struct RaycastHit hit;

        if (collisionSceneRaycast(&gCollisionScene, projectile->roomIndex, &projectile->positionDirection, COLLISION_LAYERS_STATIC | COLLISION_LAYERS_BLOCK_PORTAL, PORTAL_PROJECTILE_SPEED * FIXED_DELTA_TIME + 0.1f, 0, &hit)) {
            if (!sceneOpenPortalFromHit(
                &gScene,
                &projectile->positionDirection,
                &hit,
                &projectile->playerUp,
                i,
                projectile->roomIndex,
                1,
                0
            )) {
                effectsSplashPlay(&gScene.effects, &gFailPortalSplash[i], &hit.at, &hit.normal);
            }
            projectile->roomIndex = -1;
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

struct Vector3 gPortalGunExit = {0.0f, 97.0f, 0.0f};

void portalGunFire(struct PortalGun* portalGun, int portalIndex, struct Ray* ray, struct Transform* lookTransform, struct Vector3* playerUp, int roomIndex) {
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

    skCalculateBonePosition(&portalGun->armature, PORTAL_GUN_V_PORTALGUN_BODY_BONE, &gPortalGunExit, &projectile->effectOffset);

    projectile->effectOffset.x *= -(DEFAULT_CAMERA_FOV / (FIRST_PERSON_POV_FOV * PORTAL_GUN_SCALE));
    projectile->effectOffset.y *= (DEFAULT_CAMERA_FOV / (FIRST_PERSON_POV_FOV * PORTAL_GUN_SCALE));
    projectile->effectOffset.z *= -1.0f / PORTAL_GUN_SCALE;

    quatMultVector(&portalGun->rotation, &projectile->effectOffset, &projectile->effectOffset);

    struct Vector3 fireFrom;
    vector3Add(&projectile->effectOffset, &ray->origin, &fireFrom);

    portalTrailPlay(&projectile->trail, &fireFrom, &hit.at);
    skAnimatorRunClip(&portalGun->animator, &portal_gun_v_portalgun_Armature_fire1_clip, 0.0f, 0);
}


void portalGunFireWorld(struct PortalGun* portalGun, int portalIndex, struct Vector3* from, struct Vector3* to, int roomIndex) {
    struct PortalGunProjectile* projectile = &portalGun->projectiles[portalIndex];
    vector3Sub(to, from, &projectile->positionDirection.dir);
    vector3Normalize(&projectile->positionDirection.dir, &projectile->positionDirection.dir);
    projectile->positionDirection.origin = *from;
    projectile->roomIndex = roomIndex;
    projectile->playerUp = gUp;

    projectile->distance = 0.0f;
    projectile->maxDistance = sqrtf(vector3DistSqrd(from, to));

    projectile->effectOffset = gZeroVec;

    portalTrailPlay(&projectile->trail, from, to);
}

int portalGunIsFiring(struct PortalGun* portalGun){
    if (portalGun->shootTotalAnimationTimer > 0.0f){
        return 1;
    }
    return 0;
}

void portalGunDraw(struct PortalGun* portalGun) {
    skAnimatorRunClip(&portalGun->animator, &portal_gun_v_portalgun_Armature_draw_clip, 0.0f, 0);
}

void portalGunFizzle(struct PortalGun* portalGun) {
    skAnimatorRunClip(&portalGun->animator, &portal_gun_v_portalgun_Armature_fizzle_clip, 0.0f, 0);
}

void portalGunPickup(struct PortalGun* portalGun) {
    skAnimatorRunClip(&portalGun->animator, &portal_gun_v_portalgun_Armature_pickup_clip, 0.0f, 0);
}

void portalGunRelease(struct PortalGun* portalGun) {
    skAnimatorRunClip(&portalGun->animator, &portal_gun_v_portalgun_Armature_release_clip, 0.0f, 0);
}