#include "turret.h"

#include "decor/decor_object.h"
#include "defs.h"
#include "effects/effect_definitions.h"
#include "locales/locales.h"
#include "physics/collision_scene.h"
#include "physics/collision_capsule.h"
#include "physics/collision_tetrahedron.h"
#include "savefile/savefile.h"
#include "scene/dynamic_scene.h"
#include "util/dynamic_asset_loader.h"
#include "util/memory.h"

#include "codegen/assets/audio/clips.h"
#include "codegen/assets/audio/languages.h"
#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/dynamic_animated_model_list.h"
#include "codegen/assets/models/props/turret_01.h"

#define TURRET_MASS                2.0f
#define TURRET_COLLISION_LAYERS    (COLLISION_LAYERS_TANGIBLE | COLLISION_LAYERS_GRABBABLE | COLLISION_LAYERS_FIZZLER | COLLISION_LAYERS_BLOCK_TURRET_SHOTS)

#define TURRET_OPEN_POSITION       46.0f
#define TURRET_CLOSE_POSITION      22.0f
#define TURRET_OPEN_SPEED          4.0f
#define TURRET_SLOW_OPEN_SPEED     2.5f
#define TURRET_CLOSE_SPEED         2.0f
#define TURRET_CLOSE_DELAY         0.25f
#define TURRET_ROTATE_SPEED        2.0f
#define TURRET_SHOT_PERIOD         0.025f

#define TURRET_BULLET_DAMAGE       4.0f
#define TURRET_BULLET_IMPULSE      5.0f
#define TURRET_BULLET_PUSH_HITS    8
#define TURRET_BULLET_SOUND_HITS   4

#define TURRET_IDLE_DIALOG_DELAY   3.0f

#define TURRET_DETECT_RANGE        20.0f
#define TURRET_DETECT_FOV_DOT      0.5f   // acos(0.5) * 2 = 120 degree FOV
#define TURRET_DETECT_DELAY        0.5f

#define TURRET_DEPLOY_DELAY        0.5f

#define TURRET_SEARCH_PITCH_SPEED  0.5f
#define TURRET_SEARCH_YAW_SPEED    TURRET_SEARCH_PITCH_SPEED * 1.5f
#define TURRET_SEARCH_DURATION     5.0f
#define TURRET_SEARCH_DIALOG_DELAY 2.0f

#define TURRET_ATTACK_DIALOG_DELAY 2.0f
#define TURRET_ATTACK_FOV_DOT      0.9f   // acos(0.9) * 2 = ~50 degree FOV
#define TURRET_ATTACK_SNAP_SPEED   1.5f
#define TURRET_ATTACK_TRACK_SPEED  0.75f

#define TURRET_GRAB_MIN_ROT_DELAY  0.1f
#define TURRET_GRAB_MAX_ROT_DELAY  0.75f

#define TURRET_TIPPED_DURATION     3.0f
#define TURRET_TIPPED_ROTATE_SPEED 8.0f

#define TURRET_AUTOTIP_MAX_VEL     1.0f
#define TURRET_AUTOTIP_DIR_MAX_DOT 0.5f   // acos(0.5) * 2 = 120 degree range
#define TURRET_AUTOTIP_IMPULSE     2.5f

#define TURRET_COLLISION_BASE      0
#define TURRET_COLLISION_BODY      1

static struct CollisionTetrahedron sTurretBaseTetrahedron = {
    {0.2f, 0.45f, 0.4f}
};
struct CollisionCapsule sTurretBodyCapsule = {
    0.2f,
    0.25f
};

static struct CompoundColliderComponentDefinition sTurretColliderComponents[] = {
    [TURRET_COLLISION_BASE] = {
        {
            CollisionShapeTypeTetrahedron,
            &sTurretBaseTetrahedron,
            0.0f,
            1.0f,
            &gCollisionTetrahedronCallbacks
        },
        { 0.0f, -0.105f, -0.13f }
    },
    [TURRET_COLLISION_BODY] = {
        {
            CollisionShapeTypeCapsule,
            &sTurretBodyCapsule,
            0.0f,
            1.0f,
            &gCollisionCapsuleCallbacks
        },
        { 0.0f, 0.25f, 0.0f }
    }
};

static struct CompoundColliderDefinition sTurretCollider = {
    sTurretColliderComponents,
    sizeof(sTurretColliderComponents) / sizeof(*sTurretColliderComponents)
};

static struct Vector3 sTurretOriginOffset = { 0.0f, 0.554f, 0.0f };
static struct Vector3 sTurretLaserOffset = { 0.0f, 0.0425f, 0.11f };
static struct Vector3 sTurretMuzzleOffsets[2] = {
    { 0.15f, 0.0425f, 0.05f },  // Left
    { -0.15f, 0.0425f, 0.05f }  // Right
};

static struct Quaternion sTurretMinYaw = { 0.0f, -0.189f, 0.0f, 0.982f };
static struct Quaternion sTurretMaxYaw = { 0.0f, 0.189f, 0.0f, 0.982f };
static struct Quaternion sTurretMinPitch = { -0.23f, 0.0f, 0.0f, 0.973f };
static struct Quaternion sTurretMaxPitch = { 0.23f, 0.0f, 0.0f, 0.973f };

static short sTurretPlayerBulletHitSounds[] = {
    SOUNDS_FLESH_IMPACT_BULLET1,
    SOUNDS_FLESH_IMPACT_BULLET2,
    SOUNDS_FLESH_IMPACT_BULLET3,
    SOUNDS_FLESH_IMPACT_BULLET4,
    SOUNDS_FLESH_IMPACT_BULLET5
};

static short sTurretActiveSounds[] = {
    SOUNDS_TURRET_ACTIVE_1,
#ifdef SOUNDS_TURRET_ACTIVE_2
    SOUNDS_TURRET_ACTIVE_2,
    SOUNDS_TURRET_ACTIVE_4,
    SOUNDS_TURRET_ACTIVE_5,
    SOUNDS_TURRET_ACTIVE_6,
    SOUNDS_TURRET_ACTIVE_7,
    SOUNDS_TURRET_ACTIVE_8
#endif
};

static short sTurretAutosearchSounds[] = {
    SOUNDS_TURRET_AUTOSEARCH_1,
#ifdef SOUNDS_TURRET_AUTOSEARCH_2
    SOUNDS_TURRET_AUTOSEARCH_2,
    SOUNDS_TURRET_AUTOSEARCH_4,
    SOUNDS_TURRET_AUTOSEARCH_5,
    SOUNDS_TURRET_AUTOSEARCH_6
#endif
};

static short sTurretDeploySounds[] = {
    SOUNDS_TURRET_DEPLOY_1,
#ifdef SOUNDS_TURRET_DEPLOY_2
    SOUNDS_TURRET_DEPLOY_2,
    SOUNDS_TURRET_DEPLOY_4,
    SOUNDS_TURRET_DEPLOY_5,
    SOUNDS_TURRET_DEPLOY_6
#endif
};

static short sTurretDisabledSounds[] = {
    SOUNDS_TURRET_DISABLED_2,
#ifdef SOUNDS_TURRET_DISABLED_4
    SOUNDS_TURRET_DISABLED_4,
    SOUNDS_TURRET_DISABLED_5,
    SOUNDS_TURRET_DISABLED_6,
    SOUNDS_TURRET_DISABLED_7,
    SOUNDS_TURRET_DISABLED_8
#endif
};

static short sTurretPickupSounds[] = {
    SOUNDS_TURRET_PICKUP_1,
#ifdef SOUNDS_TURRET_PICKUP_3
    SOUNDS_TURRET_PICKUP_3,
    SOUNDS_TURRET_PICKUP_6,
    SOUNDS_TURRET_PICKUP_7,
    SOUNDS_TURRET_PICKUP_8,
    SOUNDS_TURRET_PICKUP_9,
    SOUNDS_TURRET_PICKUP_10
#endif
};

static short sTurretRetireSounds[] = {
    SOUNDS_TURRET_RETIRE_1,
#ifdef SOUNDS_TURRET_RETIRE_2
    SOUNDS_TURRET_RETIRE_2,
    SOUNDS_TURRET_RETIRE_4,
    SOUNDS_TURRET_RETIRE_5,
    SOUNDS_TURRET_RETIRE_6,
    SOUNDS_TURRET_RETIRE_7
#endif
};

static short sTurretSearchSounds[] = {
    SOUNDS_TURRET_SEARCH_1,
#ifdef SOUNDS_TURRET_SEARCH_2
    SOUNDS_TURRET_SEARCH_2,
    SOUNDS_TURRET_SEARCH_4
#endif
};

static short sTurretTippedSounds[] = {
    SOUNDS_TURRET_TIPPED_2,
#ifdef SOUNDS_TURRET_TIPPED_4
    SOUNDS_TURRET_TIPPED_4,
    SOUNDS_TURRET_TIPPED_5,
    SOUNDS_TURRET_TIPPED_6
#endif
};

#define LIST_SIZE(list) (sizeof(list) / sizeof(*list))
#define RANDOM_TURRET_SOUND(list) turretRandomSoundId(list, LIST_SIZE(list))

static short turretRandomSoundId(short* list, short listSize) {
#ifdef AUDIO_LANGUAGE_RUSSIAN
    // The Russian localization only has one sound of each type
    // Don't try to play the others (they would play in the default language)
    if (gSaveData.audio.audioLanguage == AUDIO_LANGUAGE_RUSSIAN) {
        return list[0];
    }
#endif

    return list[randomInRange(0, listSize)];
}

static void turretPlaySound(struct Turret* turret, enum TurretSoundType soundType, short soundId, short subtitleId) {
    // TODO: fix sound ID use-after-free issue
    SoundId* currentSound = &turret->currentSounds[soundType];
    soundPlayerStop(*currentSound);

    *currentSound = soundPlayerPlay(
        mapLocaleSound(soundId),
        (soundType == TurretSoundTypeDialog) ? 2.0f : 1.0f,
        0.5f,
        &turret->rigidBody.transform.position,
        &turret->rigidBody.velocity,
        SoundTypeAll
    );

    if (subtitleId != StringIdNone) {
        hudShowSubtitle(&gScene.hud, subtitleId, SubtitleTypeCaption);
    }
}

static void turretStopAllSounds(struct Turret* turret) {
    for (enum TurretSoundType t = 0; t < TurretSoundTypeCount; ++t) {
        SoundId soundId = turret->currentSounds[t];
        if (soundId == SOUND_ID_NONE) {
            continue;
        }

        soundPlayerStop(soundId);
    }
}

static void turretUpdateSounds(struct Turret* turret) {
    for (enum TurretSoundType t = 0; t < TurretSoundTypeCount; ++t) {
        SoundId soundId = turret->currentSounds[t];
        if (soundId == SOUND_ID_NONE) {
            continue;
        }

        if (soundPlayerIsPlaying(soundId)) {
            soundPlayerUpdatePosition(
                soundId,
                &turret->rigidBody.transform.position,
                &turret->rigidBody.velocity
            );
        } else {
            turret->currentSounds[t] = SOUND_ID_NONE;
        }
    }
}

static void turretRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct Turret* turret = data;

    Mtx* matrix = renderStateRequestMatrices(renderState, 1);

    if (!matrix) {
        return;
    }

    // Render without origin offset
    struct Vector3 transformedOriginOffset;
    quatMultVector(&turret->rigidBody.transform.rotation, &sTurretOriginOffset, &transformedOriginOffset);

    struct Transform transform = turret->rigidBody.transform;
    vector3Sub(&transform.position, &transformedOriginOffset, &transform.position);
    transformToMatrixL(&transform, matrix, SCENE_SCALE);

    Mtx* armature = renderStateRequestMatrices(renderState, turret->armature.numberOfBones);

    if (!armature) {
        return;
    }

    skCalculateTransforms(&turret->armature, armature);

    dynamicRenderListAddDataTouchingPortal(
        renderList,
        decorBuildFizzleGfx(turret->armature.displayList, turret->fizzleTime, renderState),
        matrix,
        turret->fizzleTime > 0.0f ? TURRET_FIZZLED_INDEX : TURRET_INDEX,
        &turret->rigidBody.transform.position,
        armature,
        turret->rigidBody.flags
    );
}

static void turretHandleCollideStartEnd(struct CollisionObject* object, struct CollisionObject* other, struct Vector3* normal) {
    struct Turret* turret = object->data;

    // Even with rounded collision, it's difficult to knock
    // a turret over with an object dropped directly on top.
    //
    // Amplify horizontal movement to make it easier.

    if (normal == NULL ||
        other->body == NULL ||
        (other->body->flags & RigidBodyIsKinematic)) {
        // Only amplify hits from free-moving physics objects
        return;
    }

    if (turret->state == TurretStateGrabbed ||
        (turret->flags & TurretFlagsTipped) ||
        vector3Dot(normal, &object->body->velocity) > (TURRET_AUTOTIP_MAX_VEL * TURRET_AUTOTIP_MAX_VEL)
    ) {
        // Only amplify when undisturbed
        return;
    }

    struct Vector3 horizontalMovement;
    vector3ProjectPlane(normal, &object->body->rotationBasis.y, &horizontalMovement);
    vector3Normalize(&horizontalMovement, &horizontalMovement);

    // Ensure push is not too far front/back
    float rightDot = vector3Dot(&horizontalMovement, &object->body->rotationBasis.x);
    if (fabsf(rightDot) < TURRET_AUTOTIP_DIR_MAX_DOT) {
        struct Vector3 sideDir = object->body->rotationBasis.x;

        if (rightDot < 0.0f) {
            vector3Negate(&sideDir, &sideDir);
        }

        vector3Lerp(&horizontalMovement, &sideDir, 0.5f, &horizontalMovement);
        vector3Normalize(&horizontalMovement, &horizontalMovement);
    }

    vector3Scale(&horizontalMovement, &horizontalMovement, object->body->mass * TURRET_AUTOTIP_IMPULSE);
    rigidBodyApplyImpulse(object->body, &object->body->transform.position, &horizontalMovement);
}

void turretInit(struct Turret* turret, struct TurretDefinition* definition) {
    compoundColliderInit(&turret->compoundCollider, &sTurretCollider, &turret->rigidBody, TURRET_COLLISION_LAYERS);
    turret->compoundCollider.children[TURRET_COLLISION_BODY].object.collideStartEnd = turretHandleCollideStartEnd;
    turret->compoundCollider.children[TURRET_COLLISION_BODY].object.data = turret;

    collisionObjectInit(&turret->collisionObject, &turret->compoundCollider.colliderType, &turret->rigidBody, TURRET_MASS, TURRET_COLLISION_LAYERS);
    collisionSceneAddDynamicObject(&turret->collisionObject);

    turret->rigidBody.transform.scale = gOneVec;
    turret->rigidBody.flags |= RigidBodyFlagsGrabbable | RigidBodyIsSleeping;

    if (definition) {
        turret->rigidBody.transform.rotation = definition->rotation;
        turret->rigidBody.currentRoom = definition->roomIndex;

        struct Vector3 originOffset;
        quatMultVector(&turret->rigidBody.transform.rotation, &sTurretOriginOffset, &originOffset);
        vector3Add(&definition->position, &originOffset, &turret->rigidBody.transform.position);
    }

    collisionObjectUpdateBB(&turret->collisionObject);

    struct SKArmatureWithAnimations* armature = dynamicAssetAnimatedModel(PROPS_TURRET_01_DYNAMIC_ANIMATED_MODEL);
    skArmatureInit(&turret->armature, armature->armature);

    laserInit(
        &turret->laser,
        &turret->rigidBody,
        &sTurretLaserOffset,
        &turret->armature.pose[PROPS_TURRET_01_ARM_C_BONE].rotation
    );

    turret->dynamicId = dynamicSceneAdd(turret, turretRender, &turret->rigidBody.transform.position, 0.75f);
    dynamicSceneSetRoomFlags(turret->dynamicId, ROOM_FLAG_FROM_INDEX(turret->rigidBody.currentRoom));

    quatIdent(&turret->targetRotation);
    turret->rotationSpeed = TURRET_ROTATE_SPEED;

    turret->playerHitCount = 0;
    turret->fizzleTime = 0.0f;
    turret->flags = 0;
    turret->openAmount = 0.0f;
    turret->shootTimer = 0.0f;
    turret->playerDetectTimer = 0.0f;

    turret->state = TurretStateIdle;
    turret->stateTimer = 0.0f;
    zeroMemory(&turret->stateData, sizeof(turret->stateData));

    for (enum TurretSoundType t = 0; t < TurretSoundTypeCount; ++t) {
        turret->currentSounds[t] = SOUND_ID_NONE;
    }
}

static uint8_t turretUpdateFizzled(struct Turret* turret) {
    enum FizzleCheckResult fizzleStatus = decorObjectUpdateFizzler(&turret->collisionObject, &turret->fizzleTime);
    if (fizzleStatus == FizzleCheckResultStart) {
        laserRemove(&turret->laser);
        compoundColliderSetCollisionLayers(&turret->collisionObject, 0);

        turretStopAllSounds(turret);
        turretPlaySound(
            turret,
            TurretSoundTypeDialog,
            SOUNDS_TURRET_FIZZLER_1,
            NPC_FLOORTURRET_TALKDISSOLVED
        );
    } else if (fizzleStatus == FizzleCheckResultEnd && turret->currentSounds[TurretSoundTypeDialog] == SOUND_ID_NONE) {
        dynamicSceneRemove(turret->dynamicId);
        collisionSceneRemoveDynamicObject(&turret->collisionObject);
        turret->dynamicId = INVALID_DYNAMIC_OBJECT;
    }

    return (turret->rigidBody.flags & RigidBodyFizzled) != 0;
}

static void turretRotationFromYawPitch(float yawAmount, float pitchAmount, struct Quaternion* rotation) {
    struct Quaternion yaw, pitch;
    quatLerp(&sTurretMinYaw, &sTurretMaxYaw, yawAmount, &yaw);
    quatLerp(&sTurretMinPitch, &sTurretMaxPitch, pitchAmount, &pitch);
    quatMultiply(&yaw, &pitch, rotation);
}

static void turretStartRotation(struct Turret* turret, float rotationSpeed) {
    turret->rotationSpeed = rotationSpeed;
    turret->flags |= TurretFlagsRotating;
}

static void turretUpdateRotation(struct Turret* turret) {
    struct Quaternion* currentRotation = &turret->armature.pose[PROPS_TURRET_01_ARM_C_BONE].rotation;

    if (quatRotateTowards(
        currentRotation,
        &turret->targetRotation,
        turret->rotationSpeed * FIXED_DELTA_TIME,
        currentRotation
    )) {
        turret->flags &= ~TurretFlagsRotating;
    }
}

static void turretApplyOpenAmount(struct Turret* turret) {
    float armPosition = mathfLerp(TURRET_CLOSE_POSITION, TURRET_OPEN_POSITION, turret->openAmount);
    turret->armature.pose[PROPS_TURRET_01_ARM_L_BONE].position.x = armPosition;
    turret->armature.pose[PROPS_TURRET_01_ARM_R_BONE].position.x = -armPosition;
}

static void turretUpdateOpenAmount(struct Turret* turret) {
    uint8_t shouldOpen = (turret->flags & TurretFlagsOpen) != 0;

    if (turret->openAmount == shouldOpen) {
        return;
    }

    if (turret->openAmount == 0.0f || turret->openAmount == 1.0f) {
        // Don't use turretPlaySound(), to allow others to play over top of this
        soundPlayerPlay(
            shouldOpen ? SOUNDS_DEPLOY : SOUNDS_RETRACT,
            1.0f,
            0.5f,
            &turret->rigidBody.transform.position,
            &turret->rigidBody.velocity,
            SoundTypeAll
        );
        hudShowSubtitle(
            &gScene.hud,
            shouldOpen ? NPC_FLOORTURRET_DEPLOY : NPC_FLOORTURRET_RETRACT,
            SubtitleTypeCaption
        );
    }

    float speed;
    if (shouldOpen) {
        if (turret->state == TurretStateDeploying) {
            speed = TURRET_SLOW_OPEN_SPEED;
        } else {
            speed = TURRET_OPEN_SPEED;
        }
    } else {
        speed = -TURRET_CLOSE_SPEED;
    }

    turret->openAmount = clampf(turret->openAmount + (speed * FIXED_DELTA_TIME), 0.0f, 1.0f);
    turretApplyOpenAmount(turret);
}

static void turretStartShooting(struct Turret* turret) {
    // Looped
    turret->currentSounds[TurretSoundTypeSfx] = soundPlayerPlay(
        SOUNDS_SHOOT1,
        5.0f,
        0.5f,
        &turret->rigidBody.transform.position,
        &turret->rigidBody.velocity,
        SoundTypeAll
    );

    turret->shootTimer = 0.0f;
    turret->flags |= TurretFlagsShooting;
}

static void turretStopShooting(struct Turret* turret) {
    soundPlayerStop(turret->currentSounds[TurretSoundTypeSfx]);
    turret->flags &= ~TurretFlagsShooting;
}

static void turretHitPlayer(struct Turret* turret, struct Player* player, struct Vector3* lookDir) {
    playerDamage(player, TURRET_BULLET_DAMAGE, PlayerDamageTypeEnemy);

    float velocityTowardTurret = vector3Dot(&player->body.velocity, lookDir);
    if (velocityTowardTurret < 0.0f ||
        (velocityTowardTurret < 0.001f && (turret->playerHitCount % TURRET_BULLET_PUSH_HITS) == 0)
    ) {
        // Always push if player is moving toward turret, otherwise periodically
        struct Vector3 push;
        vector3ProjectPlane(lookDir, &gUp, &push);
        vector3Normalize(&push, &push);
        vector3Scale(&push, &push, player->body.mass * TURRET_BULLET_IMPULSE);

        rigidBodyApplyImpulse(&player->body, &player->body.transform.position, &push);
    }

    if ((turret->playerHitCount % TURRET_BULLET_SOUND_HITS) == 0) {
        soundPlayerPlay(
            sTurretPlayerBulletHitSounds[randomInRange(0, LIST_SIZE(sTurretPlayerBulletHitSounds))],
            1.0f,
            0.5f,
            NULL,
            NULL,
            SoundTypeAll
        );
    }

    ++turret->playerHitCount;
}

static void turretUpdateShots(struct Turret* turret, struct Player* player) {
    if (turret->shootTimer > 0.0f) {
        turret->shootTimer -= FIXED_DELTA_TIME;
        return;
    }

    struct Quaternion* currentRotation = &turret->armature.pose[PROPS_TURRET_01_ARM_C_BONE].rotation;

    struct Vector3 lookDir;
    quatMultVector(currentRotation, &gForward, &lookDir);

    for (short i = 0; i < 2; ++i) {
        struct Vector3 muzzleOffset;
        quatMultVector(currentRotation, &sTurretMuzzleOffsets[i], &muzzleOffset);

        effectsSplashPlay(
            &gScene.effects,
            &gMuzzleFlash,
            &muzzleOffset,
            &lookDir,
            &turret->rigidBody.transform
        );
    }

    // TODO: bullet trails, bullet holes on objects, blood, friendly fire, etc.

    if (turret->laser.lastObjectHit == &player->collisionObject) {
        struct Vector3* laserDir = &turret->laser.beams[turret->laser.beamCount - 1].startPosition.dir;
        turretHitPlayer(turret, player, laserDir);
    }

    turret->shootTimer = TURRET_SHOT_PERIOD;
}

static uint8_t turretRaycastTarget(struct Turret* turret, struct Vector3* target, int checkPortals, struct Vector3* direction) {
    struct Vector3 turretToTarget;
    vector3Sub(target, &turret->rigidBody.transform.position, &turretToTarget);

    float targetDistance = vector3MagSqrd(&turretToTarget);
    if (targetDistance > (TURRET_DETECT_RANGE * TURRET_DETECT_RANGE) ||
        vector3Dot(&turret->rigidBody.rotationBasis.z, &turretToTarget) <= 0.0f) {
        return 0;
    }

    targetDistance = sqrtf(targetDistance);

    struct Ray ray;
    vector3Scale(&turretToTarget, &ray.dir, 1.0f / targetDistance);
    if (vector3Dot(&turret->rigidBody.rotationBasis.z, &ray.dir) < TURRET_DETECT_FOV_DOT) {
        return 0;
    }

    ray.origin = turret->rigidBody.transform.position;

    struct RaycastHit hit;
    if (collisionSceneRaycast(
            &gCollisionScene,
            turret->rigidBody.currentRoom,
            &ray,
            COLLISION_LAYERS_BLOCK_TURRET_SIGHT,
            targetDistance,
            checkPortals,
            &hit)
    ) {
        return 0;
    }

    if (checkPortals && hit.numPortalsPassed == 0) {
        return 0;
    }

    if (direction != NULL) {
        struct Quaternion rotationInv;
        quatConjugate(&turret->rigidBody.transform.rotation, &rotationInv);
        quatMultVector(&rotationInv, &turretToTarget, direction);
        vector3Negate(direction, direction);
        vector3Normalize(direction, direction);
    }

    return 1;
}

static uint8_t turretFindPlayerLineOfSight(struct Turret* turret, struct Player* player, struct Vector3* direction) {
    if (playerIsDead(player)) {
        return 0;
    }

    struct Vector3 target = player->body.transform.position;
    vector3AddScaled(&target, &turret->rigidBody.rotationBasis.y, -PLAYER_HEAD_HEIGHT / 2.0f, &target);

    if (turretRaycastTarget(turret, &target, 0, direction)) {
        return 1;
    }

    if (collisionSceneIsPortalOpen()) {
        for (int i = 0; i < 2; ++i) {
            struct Vector3 portalTarget;
            transformPointInverseNoScale(gCollisionScene.portalTransforms[i], &target, &portalTarget);
            transformPointNoScale(gCollisionScene.portalTransforms[1 - i], &portalTarget, &portalTarget);

            if (turretRaycastTarget(turret, &portalTarget, 1, direction)) {
                return 1;
            }
        }
    }

    return 0;
}

// State transition functions

static void turretEnterSearching(struct Turret* turret) {
    turret->stateTimer = TURRET_SEARCH_DURATION;
    turret->stateData.searching.yawDirection = mathfRandomFloat() > 0.5f;
    turret->stateData.searching.pitchDirection = mathfRandomFloat() > 0.5f;
    turret->state = TurretStateSearching;
}

static void turretEnterClosing(struct Turret* turret, enum TurretState nextState, float nextStateTimer, short soundId, short subtitleId) {
    quatIdent(&turret->targetRotation);
    turretStartRotation(turret, TURRET_ROTATE_SPEED);

    struct ClosingStateData* state = &turret->stateData.closing;
    state->nextState = nextState;
    state->nextStateTimer = nextStateTimer;
    state->soundId = soundId;
    state->subtitleId = subtitleId;

    turret->stateTimer = TURRET_CLOSE_DELAY;
    turret->state = TurretStateClosing;
}

static void turretCheckPlayerDetected(struct Turret* turret, struct Player* player, enum TurretState nextState) {
    if (!turretFindPlayerLineOfSight(turret, player, NULL)) {
        turret->playerDetectTimer = 0.0f;
    } else if (turret->playerDetectTimer < TURRET_DETECT_DELAY) {
        turret->playerDetectTimer += FIXED_DELTA_TIME;
    } else {
        turret->playerDetectTimer = 0.0f;
        turret->stateTimer = TURRET_DEPLOY_DELAY;
        turret->state = nextState;
    }
}

static void turretCheckGrabbed(struct Turret* turret, struct Player* player) {
    if (playerIsGrabbingObject(player, &turret->collisionObject)) {
        turretPlaySound(
            turret,
            TurretSoundTypeDialog,
            RANDOM_TURRET_SOUND(sTurretPickupSounds),
            NPC_FLOORTURRET_TALKPICKUP
        );
        hudShowSubtitle(&gScene.hud, NPC_FLOORTURRET_ACTIVATE, SubtitleTypeCaption);

        turretStopShooting(turret);

        turret->flags |= TurretFlagsOpen;
        turret->stateTimer = 0.0f;
        turret->state = TurretStateGrabbed;
    }
}

static void turretCheckTipped(struct Turret* turret) {
    if (turret->rigidBody.flags & RigidBodyIsSleeping) {
        return;
    }

    if (vector3Dot(&turret->rigidBody.rotationBasis.y, &gUp) <= 0.0f) {
        turretPlaySound(
            turret,
            TurretSoundTypeDialog,
            RANDOM_TURRET_SOUND(sTurretTippedSounds),
            NPC_FLOORTURRET_TALKTIPPED
        );

        if (!(turret->flags & TurretFlagsShooting)) {
            turretStartShooting(turret);
        }

        turret->flags = (turret->flags & ~TurretFlagsRotating) | TurretFlagsOpen | TurretFlagsTipped;
        turret->stateTimer = TURRET_TIPPED_DURATION;
        turret->state = TurretStateTipped;
    }
}

// State handler functions

static void turretUpdateIdle(struct Turret* turret, struct Player* player) {
    if (turret->stateTimer > 0.0f) {
        turret->stateTimer -= FIXED_DELTA_TIME;

        if (turret->stateTimer <= 0.0f) {
            turretPlaySound(
                turret,
                TurretSoundTypeDialog,
                RANDOM_TURRET_SOUND(sTurretAutosearchSounds),
                NPC_FLOORTURRET_TALKAUTOSEARCH
            );
        }
    }

    turretCheckPlayerDetected(turret, player, TurretStateDeploying);
    turretCheckTipped(turret);
    turretCheckGrabbed(turret, player);
}

static void turretUpdateDeploying(struct Turret* turret, struct Player* player) {
    if (turret->stateTimer <= 0.0f) {
        if (!(turret->flags & TurretFlagsOpen)) {
            turretPlaySound(
                turret,
                TurretSoundTypeDialog,
                RANDOM_TURRET_SOUND(sTurretDeploySounds),
                NPC_FLOORTURRET_TALKDEPLOY
            );

            turret->flags |= TurretFlagsOpen;
        } else if (turret->openAmount == 1.0f) {
            turret->stateTimer = 0.0f;
            turret->state = TurretStateAttacking;
        }
    } else {
        turret->stateTimer -= FIXED_DELTA_TIME;
    }

    turretCheckTipped(turret);
    turretCheckGrabbed(turret, player);
}

static void turretUpdateSearching(struct Turret* turret, struct Player* player) {
    struct SearchingStateData* state = &turret->stateData.searching;

    if (!(turret->flags & TurretFlagsRotating)) {
        // Rotate back and forth on each axis at different speeds
        // Done without keyframed animation since it's smaller and smoother
        if (state->yawAmount <= 0.0f || state->yawAmount >= 1.0f) {
            state->yawDirection ^= 1;
        }
        if (state->pitchAmount <= 0.0f || state->pitchAmount >= 1.0f) {
            state->pitchDirection ^= 1;
        }
        state->yawAmount += (state->yawDirection ? 1 : -1) * TURRET_SEARCH_YAW_SPEED * FIXED_DELTA_TIME;
        state->pitchAmount += (state->pitchDirection ? 1 : -1) * TURRET_SEARCH_PITCH_SPEED * FIXED_DELTA_TIME;

        struct Quaternion* currentRotation = &turret->armature.pose[PROPS_TURRET_01_ARM_C_BONE].rotation;
        turretRotationFromYawPitch(state->yawAmount, state->pitchAmount, currentRotation);
    }

    // Crossed second boundary?
    int currentSecond = (int)turret->stateTimer;
    if (currentSecond < (int)(turret->stateTimer + FIXED_DELTA_TIME)) {
        turretPlaySound(
            turret,
            TurretSoundTypeSfx,
            SOUNDS_PING,
            StringIdNone  // Don't spam
        );

        if (currentSecond == (int)(TURRET_SEARCH_DURATION - TURRET_SEARCH_DIALOG_DELAY)) {
            turretPlaySound(
                turret,
                TurretSoundTypeDialog,
                RANDOM_TURRET_SOUND(sTurretSearchSounds),
                NPC_FLOORTURRET_TALKSEARCH
            );
        }
    }

    if (turret->stateTimer <= 0.0f) {
        turretEnterClosing(
            turret,
            TurretStateIdle,
            TURRET_IDLE_DIALOG_DELAY,
            RANDOM_TURRET_SOUND(sTurretRetireSounds),
            NPC_FLOORTURRET_TALKRETIRE
        );
    } else {
        turret->stateTimer -= FIXED_DELTA_TIME;
    }

    turretCheckPlayerDetected(turret, player, TurretStateAttacking);
    turretCheckTipped(turret);
    turretCheckGrabbed(turret, player);
}

static void turretUpdateAttacking(struct Turret* turret, struct Player* player) {
    // Talk while shooting
    if (turret->currentSounds[TurretSoundTypeDialog] == SOUND_ID_NONE &&
        (turret->flags & TurretFlagsShooting)) {
        if (turret->stateTimer <= 0.0f) {
            turretPlaySound(
                turret,
                TurretSoundTypeDialog,
                RANDOM_TURRET_SOUND(sTurretActiveSounds),
                NPC_FLOORTURRET_TALKACTIVE
            );

            turret->stateTimer = TURRET_ATTACK_DIALOG_DELAY;
        } else {
            turret->stateTimer -= FIXED_DELTA_TIME;
        }
    }

    // Rotate to player
    if (!(turret->flags & TurretFlagsRotating)) {
        struct Vector3 playerDirection;
        if (turretFindPlayerLineOfSight(turret, player, &playerDirection)) {
            struct Quaternion* currentRotation = &turret->armature.pose[PROPS_TURRET_01_ARM_C_BONE].rotation;
            struct Vector3 lookDir;
            quatMultVector(currentRotation, &gForward, &lookDir);

            // Only shoot if aiming near player
            float rotationSpeed;
            if (vector3Dot(&lookDir, &playerDirection) <= -TURRET_ATTACK_FOV_DOT) {
                rotationSpeed = TURRET_ATTACK_TRACK_SPEED;
                if (!(turret->flags & TurretFlagsShooting)) {
                    turretStartShooting(turret);
                }
            } else {
                rotationSpeed = TURRET_ATTACK_SNAP_SPEED;
                turretStopShooting(turret);
            }

            quatLook(&playerDirection, &gUp, &turret->targetRotation);
            turretStartRotation(turret, rotationSpeed);
        } else {
            turretStopShooting(turret);

            // Recenter
            turret->stateData.searching.yawAmount = 0.5f;
            turret->stateData.searching.pitchAmount = 0.5f;
            quatIdent(&turret->targetRotation);
            turretStartRotation(turret, TURRET_ROTATE_SPEED);

            turretEnterSearching(turret);
        }
    }

    turretCheckTipped(turret);
    turretCheckGrabbed(turret, player);
}

static void turretUpdateGrabbed(struct Turret* turret, struct Player* player) {
    // Look in a random direction, pause, repeat
    if (!(turret->flags & TurretFlagsRotating)) {
        if (turret->stateTimer <= 0.0f) {
            // Save so we can enter the searching state at the same rotation
            // Not critical, but the searching state uses them regardless
            struct GrabbedStateData* state = &turret->stateData.grabbed;
            state->yawAmount = mathfRandomFloat();
            state->pitchAmount = mathfRandomFloat();

            turretRotationFromYawPitch(state->yawAmount, state->pitchAmount, &turret->targetRotation);
            turretStartRotation(turret, TURRET_ROTATE_SPEED);

            turretPlaySound(
                turret,
                TurretSoundTypeSfx,
                SOUNDS_ACTIVE,
                StringIdNone  // Don't spam
            );

            turret->stateTimer = randomInRangef(
                TURRET_GRAB_MIN_ROT_DELAY,
                TURRET_GRAB_MAX_ROT_DELAY
            );
        } else {
            turret->stateTimer -= FIXED_DELTA_TIME;
        }
    }

    if (!playerIsGrabbingObject(player, &turret->collisionObject)) {
        turretEnterSearching(turret);
    }
}

static void turretUpdateTipped(struct Turret* turret) {
    if (!(turret->flags & TurretFlagsRotating)) {
        turretRotationFromYawPitch(mathfRandomFloat(), mathfRandomFloat(), &turret->targetRotation);
        turretStartRotation(turret, TURRET_TIPPED_ROTATE_SPEED);
    }

    if (turret->stateTimer <= 0.0f) {
        turretStopShooting(turret);
        turretEnterClosing(
            turret,
            TurretStateDying,
            0.0f,
            RANDOM_TURRET_SOUND(sTurretDisabledSounds),
            NPC_FLOORTURRET_TALKDISABLED
        );
    } else {
        turret->stateTimer -= FIXED_DELTA_TIME;
    }
}

static void turretUpdateClosing(struct Turret* turret, struct Player* player) {
    struct ClosingStateData* state = &turret->stateData.closing;

    if (turret->stateTimer <= 0.0f) {
        if (turret->flags & TurretFlagsOpen) {
            turretPlaySound(
                turret,
                TurretSoundTypeDialog,
                state->soundId,
                state->subtitleId
            );

            turret->flags &= ~TurretFlagsOpen;
        } else if (turret->openAmount == 0.0f) {
            turret->stateTimer = state->nextStateTimer;
            turret->state = state->nextState;
        }
    } else {
        turret->stateTimer -= FIXED_DELTA_TIME;
    }

    if (state->nextState == TurretStateIdle) {
        turretCheckTipped(turret);
        turretCheckGrabbed(turret, player);
    }
}

static void turretUpdateDying(struct Turret* turret) {
    // TODO: fade eye

    if (turret->currentSounds[TurretSoundTypeDialog] == SOUND_ID_NONE) {
        laserRemove(&turret->laser);
        turret->state = TurretStateDead;
    }
}

int turretUpdate(struct Turret* turret, struct Player* player) {
    if (turret->dynamicId == INVALID_DYNAMIC_OBJECT) {
        return 0;
    }

    if (turret->collisionObject.flags & COLLISION_OBJECT_PLAYER_STANDING) {
        turret->collisionObject.flags &= ~COLLISION_OBJECT_PLAYER_STANDING;
    }
    dynamicSceneSetRoomFlags(turret->dynamicId, ROOM_FLAG_FROM_INDEX(turret->rigidBody.currentRoom));

    turretUpdateSounds(turret);

    if (turretUpdateFizzled(turret)) {
        return 1;
    }

    switch (turret->state) {
        case TurretStateIdle:
            turretUpdateIdle(turret, player);
            break;
        case TurretStateDeploying:
            turretUpdateDeploying(turret, player);
            break;
        case TurretStateSearching:
            turretUpdateSearching(turret, player);
            break;
        case TurretStateAttacking:
            turretUpdateAttacking(turret, player);
            break;
        case TurretStateGrabbed:
            turretUpdateGrabbed(turret, player);
            break;
        case TurretStateTipped:
            turretUpdateTipped(turret);
            break;
        case TurretStateClosing:
            turretUpdateClosing(turret, player);
            break;
        case TurretStateDying:
            turretUpdateDying(turret);
            break;
        default:
        case TurretStateDead:
            break;
    }

    turretUpdateOpenAmount(turret);

    if (turret->flags & TurretFlagsRotating) {
        turretUpdateRotation(turret);
    }
    if (turret->flags & TurretFlagsShooting) {
        turretUpdateShots(turret, player);
    }

    return 1;
}

struct Turret* turretNew(struct TurretDefinition* definition) {
    struct Turret* turret = malloc(sizeof(struct Turret));
    turretInit(turret, definition);
    return turret;
}

void turretDelete(struct Turret* turret) {
    // We only delete if the turret was fizzled, in which case it cleans up itself
    // Just need to delete
    free(turret);
}

struct Quaternion* turretGetLookRotation(struct Turret* turret) {
    return &turret->armature.pose[PROPS_TURRET_01_ARM_C_BONE].rotation;
}

void turretOnDeserialize(struct Turret* turret) {
    turretApplyOpenAmount(turret);

    if (turret->flags & TurretFlagsShooting) {
        // Restart looped shooting sound
        turretStartShooting(turret);
    }

    if ((turret->rigidBody.flags & RigidBodyFizzled) ||
        turret->state == TurretStateDead) {
        laserRemove(&turret->laser);
    }
}
