#include "portal_gun.h"

#include "effects/effect_definitions.h"
#include "levels/material_state.h"
#include "math/math.h"
#include "physics/collision_scene.h"
#include "physics/collision_cylinder.h"
#include "render_plan.h"
#include "scene.h"
#include "util/frame_time.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/grav_flare.h"
#include "codegen/assets/models/portal_gun/v_portalgun.h"

#define PORTAL_GUN_SCALE                    512.0f
#define PORTAL_GUN_SHOOT_TIME               0.44f
#define PORTAL_GUN_MAX_ROT_AMOUNT           0.45f
#define PORTAL_GUN_NEAR_PLANE               0.05f
#define PORTAL_GUN_FAR_PLANE                4.0f

#define PORTAL_PROJECTILE_RADIUS            0.15f
#define PORTAL_PROJECTILE_FADE_IN_DISTANCE  5.0f
#define PORTAL_PROJECTILE_COLLISION_LAYERS  (COLLISION_LAYERS_STATIC | COLLISION_LAYERS_BLOCK_PORTAL)
#define PORTAL_PROJECTILE_HIT_BIAS          0.1f  // Avoids false negatives at end of path
#define PORTAL_PROJECTILE_MAX_DISTANCE      50.0f

static struct Quaternion sFlipAroundY = {0.0f, 1.0f, 0.0f, 0.0f};

static struct Transform sGunTransform = {
    {0.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f, 0.0f},
    {1.0f, 1.0f, 1.0f},
};

static struct Vector3 sGunBarrelOffset = {0.0f, 97.0f, 0.0f};

static struct Coloru8 sProjectileColors[] = {
    {200, 100, 50, 255},
    {50, 70, 200, 255},
};

void portalGunInit(struct PortalGun* portalGun, struct Player* player, float fov, int isFreshStart) {
    portalTrailInit(&portalGun->projectiles[0].trail);
    portalGun->projectiles[0].roomIndex = ROOM_NONE;

    portalTrailInit(&portalGun->projectiles[1].trail);
    portalGun->projectiles[1].roomIndex = ROOM_NONE;

    skArmatureInit(&portalGun->armature, &portal_gun_v_portalgun_armature);
    skAnimatorInit(&portalGun->animator, portal_gun_v_portalgun_armature.numberOfBones);

    if (isFreshStart) {
        skAnimatorRunClip(&portalGun->animator, &portal_gun_v_portalgun_Armature_draw_clip, 0.0f, SKAnimatorStartFlagsLoadSync);
    } else {
        skAnimatorRunClip(&portalGun->animator, &portal_gun_v_portalgun_Armature_idle_clip, 0.0f, SKAnimatorStartFlagsLoadSync);
    }

    skAnimatorUpdate(&portalGun->animator, portalGun->armature.pose, FIXED_DELTA_TIME);

    portalGun->rotation = player->lookTransform.rotation;
    portalGun->portalGunVisible = (player->flags & (PlayerHasFirstPortalGun | PlayerHasSecondPortalGun));
    portalGun->shootTimer = 0.0f;
    portalGun->fov = fov;
}

static void portalGunUpdateRotation(struct PortalGun* portalGun, struct Player* player) {
    if (player->passedThroughPortal) {
        int portalIndex = player->passedThroughPortal - 1;

        struct Transform* transform = collisionSceneTransformToOtherPortal(portalIndex);
        if (transform) {
            struct Quaternion newRotation;
            quatMultiply(&transform->rotation, &portalGun->rotation, &newRotation);
            portalGun->rotation = newRotation;
        }
    }

    quatLerp(
        &portalGun->rotation,
        &player->lookTransform.rotation,
        PORTAL_GUN_MAX_ROT_AMOUNT,
        &portalGun->rotation
    );
}

static void portalGunProjectileUpdate(struct PortalGun* portalGun, int portalIndex) {
    struct PortalGunProjectile* projectile = &portalGun->projectiles[portalIndex];

    portalTrailUpdate(&projectile->trail);

    if (projectile->roomIndex == ROOM_NONE) {
        return;
    } else if (projectile->distance >= projectile->maxDistance) {
        projectile->roomIndex = ROOM_NONE;
        return;
    }

    struct RaycastHit hit;
    if (collisionSceneRaycast(
        &gCollisionScene,
        projectile->roomIndex,
        &projectile->positionDirection,
        PORTAL_PROJECTILE_COLLISION_LAYERS,
        (PORTAL_TRAIL_SPEED * FIXED_DELTA_TIME) + PORTAL_PROJECTILE_HIT_BIAS,
        0, // passThroughPortals
        &hit
    )) {
        projectile->roomIndex = ROOM_NONE;

        if (!sceneOpenPortalFromHit(
            &gScene,
            &projectile->positionDirection,
            &hit,
            &projectile->upDir,
            portalIndex,
            1, // fromPlayer
            0  // validateOnly
        )) {
            effectsParticlePlay(
                &gScene.effects,
                &gFailPortalSplash[portalIndex],
                &hit.at,
                &hit.normal,
                NULL,
                ROOM_NONE,
                0
            );
        }
    } else {
        projectile->roomIndex = hit.roomIndex;

        vector3AddScaled(
            &projectile->positionDirection.origin,
            &projectile->positionDirection.dir,
            PORTAL_TRAIL_SPEED * FIXED_DELTA_TIME,
            &projectile->positionDirection.origin
        );
        projectile->distance += PORTAL_TRAIL_SPEED * FIXED_DELTA_TIME;
    }
}

void portalGunUpdate(struct PortalGun* portalGun, struct Player* player) {
    skAnimatorUpdate(&portalGun->animator, portalGun->armature.pose, FIXED_DELTA_TIME);

    portalGunUpdateRotation(portalGun, player);
    portalGun->portalGunVisible = (player->flags & (PlayerHasFirstPortalGun | PlayerHasSecondPortalGun));

    if (portalGun->shootTimer > 0.0f) {
        portalGun->shootTimer -= FIXED_DELTA_TIME;
        if (portalGun->shootTimer <= 0.0f){
            portalGun->shootTimer = 0.0f;
        }
    }

    for (int i = 0; i < 2; ++i) {
        portalGunProjectileUpdate(portalGun, i);
    }
}

void portalGunOnDeserialize(struct PortalGun* portalGun, struct Player* player) {
    if (player->flags & (PlayerHasFirstPortalGun | PlayerHasSecondPortalGun)) {
        portalGun->rotation = player->lookTransform.rotation;
    }
}

static void portalGunProjectileRender(
    struct PortalGunProjectile* projectile,
    int portalIndex,
    struct RenderState* renderState,
    struct MaterialState* materialState,
    struct Transform* fromView
) {
    struct Transform transform;

    if (projectile->distance < projectile->maxDistance) {
        // Move from gun to target
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
    if (!mtx) {
        return;
    }

    transformToMatrixL(&transform, mtx, SCENE_SCALE);

    if (projectile->distance == 0.0f) {
        // Muzzle flash
        materialStateSet(materialState, PORTAL_2_PARTICLE_INDEX, renderState);
    } else {
        // Projectile
        materialStateSet(materialState, BRIGHTGLOW_Y_INDEX, renderState);

        struct Coloru8* color = &sProjectileColors[portalIndex];
        float alpha = minf(255.0f, projectile->distance * (255.0f / PORTAL_PROJECTILE_FADE_IN_DISTANCE));
        gDPSetPrimColor(renderState->dl++, 255, 255, color->r, color->g, color->b, (u8)alpha);
    }

    gSPMatrix(renderState->dl++, mtx, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    gSPDisplayList(renderState->dl++, grav_flare_model_gfx);
    gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
}

extern LookAt gLookAt;
extern float getAspect();

void portalGunRender(
    struct PortalGun* portalGun,
    struct RenderState* renderState,
    struct Camera* fromCamera,
    int lastFiredIndex
) {
    struct MaterialState materialState;
    materialStateInit(&materialState, DEFAULT_INDEX);

    for (int i = 0; i < 2; ++i) {
        struct PortalGunProjectile* projectile = &portalGun->projectiles[i];

        portalTrailRender(&projectile->trail, renderState, &materialState, fromCamera, i);

        if (projectile->roomIndex != ROOM_NONE) {
            portalGunProjectileRender(projectile, i, renderState, &materialState, &fromCamera->transform);
        }
    }

    if (!portalGun->portalGunVisible) {
        return;
    }

    Mtx* mtx = renderStateRequestMatrices(renderState, 2);
    if (!mtx) {
        return;
    }

    u16 perspectiveNormalize;
    guPerspective(
        &mtx[0],
        &perspectiveNormalize,
        portalGun->fov,
        getAspect(),
        PORTAL_GUN_NEAR_PLANE * SCENE_SCALE,
        PORTAL_GUN_FAR_PLANE * SCENE_SCALE,
        1.0f
    );
    gSPMatrix(renderState->dl++, &mtx[0], G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);

    gDPPipeSync(renderState->dl++);

    // Set the portal indicator color
    if (lastFiredIndex >= 0 && lastFiredIndex <= 1) {
        struct Coloru8 color = sProjectileColors[lastFiredIndex];
        gDPSetEnvColor(renderState->dl++, color.r, color.g, color.b, 255);
    } else {
        gDPSetEnvColor(renderState->dl++, 128, 128, 128, 255);
    }

    struct Quaternion inverseCameraRotation;
    struct Quaternion relativeRotation;
    quatConjugate(&fromCamera->transform.rotation, &inverseCameraRotation);
    quatMultiply(&inverseCameraRotation, &portalGun->rotation, &relativeRotation);
    quatMultiply(&relativeRotation, &sFlipAroundY, &sGunTransform.rotation);

    // Reflection
    LookAt* lookAt = renderStateRequestLookAt(renderState);
    if (!lookAt) {
        return;
    }

    *lookAt = gLookAt;

    struct Vector3 lookDirection;
    quatMultVector(&inverseCameraRotation, &gForward, &lookDirection);
    vector3ToVector3u8(&lookDirection, (struct Vector3u8*)&lookAt->l[0].l.dir);

    quatMultVector(&inverseCameraRotation, &gUp, &lookDirection);
    vector3ToVector3u8(&lookDirection, (struct Vector3u8*)&lookAt->l[1].l.dir);

    gSPLookAt(renderState->dl++, &gLookAt);
    gSPLookAt(renderState->dl++, lookAt);

    // Gun
    transformToMatrixL(&sGunTransform, &mtx[1], PORTAL_GUN_SCALE);
    gSPMatrix(renderState->dl++, &mtx[1], G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL);
    skRenderObject(&portalGun->armature, NULL, renderState);
    gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);

    gSPDisplayList(renderState->dl++, static_default);
}

void portalGunFire(
    struct PortalGun* portalGun,
    int portalIndex,
    struct Ray* ray,
    struct Transform* lookTransform,
    struct Vector3* upDir,
    int roomIndex
) {
    struct PortalGunProjectile* projectile = &portalGun->projectiles[portalIndex];

    struct RaycastHit hit;
    if (!collisionSceneRaycast(
        &gCollisionScene,
        roomIndex,
        ray,
        PORTAL_PROJECTILE_COLLISION_LAYERS,
        PORTAL_PROJECTILE_MAX_DISTANCE,
        0, // passThroughPortals
        &hit)
    ) {
        vector3AddScaled(&ray->origin, &ray->dir, hit.distance, &hit.at);
    }

    projectile->positionDirection = *ray;
    projectile->upDir = *upDir;
    projectile->distance = 0.0f;
    projectile->maxDistance = hit.distance;
    projectile->roomIndex = roomIndex;

    skCalculateBonePosition(&portalGun->armature, PORTAL_GUN_V_PORTALGUN_BODY_BONE, &sGunBarrelOffset, &projectile->effectOffset);
    projectile->effectOffset.x *= -(gScene.camera.fov / (portalGun->fov * PORTAL_GUN_SCALE));
    projectile->effectOffset.y *= (gScene.camera.fov / (portalGun->fov * PORTAL_GUN_SCALE));
    projectile->effectOffset.z *= -1.0f / PORTAL_GUN_SCALE;
    quatMultVector(&portalGun->rotation, &projectile->effectOffset, &projectile->effectOffset);

    struct Vector3 fireFrom;
    vector3Add(&projectile->effectOffset, &ray->origin, &fireFrom);

    portalTrailPlay(&projectile->trail, &fireFrom, &hit.at);
    skAnimatorRunClip(&portalGun->animator, &portal_gun_v_portalgun_Armature_fire1_clip, 0.0f, 0);

    portalGun->shootTimer = PORTAL_GUN_SHOOT_TIME;
}

void portalGunFireWorld(
    struct PortalGun* portalGun,
    int portalIndex,
    struct Vector3* from,
    struct Vector3* to,
    int roomIndex
) {
    struct PortalGunProjectile* projectile = &portalGun->projectiles[portalIndex];

    struct Vector3 dir;
    vector3Sub(to, from, &dir);
    vector3Normalize(&dir, &projectile->positionDirection.dir);

    projectile->positionDirection.origin = *from;
    projectile->upDir = gUp;
    projectile->effectOffset = gZeroVec;
    projectile->distance = 0.0f;
    projectile->maxDistance = vector3Dot(&dir, &projectile->positionDirection.dir);
    projectile->roomIndex = roomIndex;

    portalTrailPlay(&projectile->trail, from, to);
}

int portalGunIsFiring(struct PortalGun* portalGun) {
    return portalGun->shootTimer > 0.0f;
}

void portalGunPlayDraw(struct PortalGun* portalGun) {
    skAnimatorRunClip(&portalGun->animator, &portal_gun_v_portalgun_Armature_draw_clip, 0.0f, 0);
}

void portalGunPlayFizzle(struct PortalGun* portalGun) {
    skAnimatorRunClip(&portalGun->animator, &portal_gun_v_portalgun_Armature_fizzle_clip, 0.0f, 0);
}

void portalGunPlayPickup(struct PortalGun* portalGun) {
    skAnimatorRunClip(&portalGun->animator, &portal_gun_v_portalgun_Armature_pickup_clip, 0.0f, 0);
}

void portalGunPlayRelease(struct PortalGun* portalGun) {
    skAnimatorRunClip(&portalGun->animator, &portal_gun_v_portalgun_Armature_release_clip, 0.0f, 0);
}
