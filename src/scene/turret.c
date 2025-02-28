#include "turret.h"

#include "decor/decor_object.h"
#include "defs.h"
#include "locales/locales.h"
#include "physics/collision_scene.h"
#include "savefile/savefile.h"
#include "scene/dynamic_scene.h"
#include "util/dynamic_asset_loader.h"
#include "util/memory.h"

#include "codegen/assets/audio/clips.h"
#include "codegen/assets/audio/languages.h"
#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/dynamic_animated_model_list.h"
#include "codegen/assets/models/props/turret_01.h"

static struct CollisionBox gTurretCollisionBox = {
    {0.2f, 0.554f, 0.4f}
};

static struct ColliderTypeData gTurretCollider = {
    CollisionShapeTypeBox,
    &gTurretCollisionBox,
    0.0f,
    1.0f,
    &gCollisionBoxCallbacks
};

static struct Vector3 sTurretOriginOffset = { 0.0f, 0.554f, 0.0f };
static struct Vector3 sTurretLaserOffset = { 0.0f, 0.0425f, 0.11f };

static struct Quaternion sTurretMinYaw = { 0.0f, -0.189f, 0.0f, 0.982f };
static struct Quaternion sTurretMaxYaw = { 0.0f, 0.189f, 0.0f, 0.982f };
static struct Quaternion sTurretMinPitch = { -0.23f, 0.0f, 0.0f, 0.973f };
static struct Quaternion sTurretMaxPitch = { 0.23f, 0.0f, 0.0f, 0.973f };

static short sTurretDisabledSounds[] = {
    SOUNDS_TURRET_DISABLED_2,
    SOUNDS_TURRET_DISABLED_4,
    SOUNDS_TURRET_DISABLED_5,
    SOUNDS_TURRET_DISABLED_6,
    SOUNDS_TURRET_DISABLED_7,
    SOUNDS_TURRET_DISABLED_8
};

static short sTurretPickupSounds[] = {
    SOUNDS_TURRET_PICKUP_1,
    SOUNDS_TURRET_PICKUP_3,
    SOUNDS_TURRET_PICKUP_6,
    SOUNDS_TURRET_PICKUP_7,
    SOUNDS_TURRET_PICKUP_8,
    SOUNDS_TURRET_PICKUP_9,
    SOUNDS_TURRET_PICKUP_10
};

static short sTurretRetireSounds[] = {
    SOUNDS_TURRET_RETIRE_1,
    SOUNDS_TURRET_RETIRE_2,
    SOUNDS_TURRET_RETIRE_4,
    SOUNDS_TURRET_RETIRE_5,
    SOUNDS_TURRET_RETIRE_6,
    SOUNDS_TURRET_RETIRE_7,
};

static short sTurretSearchSounds[] = {
    SOUNDS_TURRET_SEARCH_1,
    SOUNDS_TURRET_SEARCH_2,
    SOUNDS_TURRET_SEARCH_4
};

static short sTurretTippedSounds[] = {
    SOUNDS_TURRET_TIPPED_2,
    SOUNDS_TURRET_TIPPED_4,
    SOUNDS_TURRET_TIPPED_5,
    SOUNDS_TURRET_TIPPED_6,
};

#define TURRET_MASS                3.0f
#define TURRET_COLLISION_LAYERS    (COLLISION_LAYERS_TANGIBLE | COLLISION_LAYERS_GRABBABLE | COLLISION_LAYERS_FIZZLER | COLLISION_LAYERS_BLOCK_TURRET_SHOTS)

#define TURRET_OPEN_POSITION       46.0f
#define TURRET_CLOSE_POSITION      22.0f
#define TURRET_OPEN_SPEED          4.0f
#define TURRET_CLOSE_SPEED         2.0f
#define TURRET_CLOSE_DELAY         0.25f
#define TURRET_ROTATE_SPEED        2.0f

#define TURRET_SEARCH_PITCH_SPEED  0.5f
#define TURRET_SEARCH_YAW_SPEED    TURRET_SEARCH_PITCH_SPEED * 1.5f
#define TURRET_SEARCH_DURATION     5.0f
#define TURRET_SEARCH_DIALOG_DELAY 2.0f

#define TURRET_GRAB_MIN_ROT_DELAY  0.1f
#define TURRET_GRAB_MAX_ROT_DELAY  0.75f

#define TURRET_TIPPED_DURATION     3.0f
#define TURRET_TIPPED_ROTATE_SPEED 8.0f

#define RANDOM_TURRET_SOUND(list) turretRandomSoundId(list, sizeof(list) / sizeof(*list))

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
    struct Turret* turret = (struct Turret*)data;

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

void turretInit(struct Turret* turret, struct TurretDefinition* definition) {
    turret->definition = definition;

    collisionObjectInit(&turret->collisionObject, &gTurretCollider, &turret->rigidBody, TURRET_MASS, TURRET_COLLISION_LAYERS);
    collisionSceneAddDynamicObject(&turret->collisionObject);

    turret->rigidBody.transform.rotation = definition->rotation;
    turret->rigidBody.transform.scale = gOneVec;
    turret->rigidBody.flags |= RigidBodyFlagsGrabbable;
    turret->rigidBody.currentRoom = definition->roomIndex;

    struct Vector3 originOffset;
    quatMultVector(&turret->rigidBody.transform.rotation, &sTurretOriginOffset, &originOffset);
    vector3Add(&definition->position, &originOffset, &turret->rigidBody.transform.position);

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

    turret->fizzleTime = 0.0f;
    turret->flags = 0;
    turret->openAmount = 0.0f;

    turret->state = TurretStateIdle;
    turret->stateTimer = 0.0f;
    zeroMemory(&turret->stateData, sizeof(turret->stateData));

    for (enum TurretSoundType t = 0; t < TurretSoundTypeCount; ++t) {
        turret->currentSounds[t] = SOUND_ID_NONE;
    }
}

// State handler functions

static uint8_t turretUpdateFizzled(struct Turret* turret) {
    enum FizzleCheckResult fizzleStatus = decorObjectUpdateFizzler(&turret->collisionObject, &turret->fizzleTime);
    if (fizzleStatus == FizzleCheckResultStart) {
        laserRemove(&turret->laser);

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

static void turretStartClose(struct Turret* turret, enum TurretState nextState, short soundId, short subtitleId) {
    quatIdent(&turret->targetRotation);
    turretStartRotation(turret, TURRET_ROTATE_SPEED);

    struct ClosingStateData* state = &turret->stateData.closing;
    state->nextState = nextState;
    state->soundId = soundId;
    state->subtitleId = subtitleId;

    turret->stateTimer = TURRET_CLOSE_DELAY;
    turret->state = TurretStateClosing;
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

    float speed = shouldOpen ? TURRET_OPEN_SPEED : -TURRET_CLOSE_SPEED;
    turret->openAmount = clampf(turret->openAmount + (speed * FIXED_DELTA_TIME), 0.0f, 1.0f);

    float armPosition = mathfLerp(TURRET_CLOSE_POSITION, TURRET_OPEN_POSITION, turret->openAmount);
    turret->armature.pose[PROPS_TURRET_01_ARM_L_BONE].position.x = armPosition;
    turret->armature.pose[PROPS_TURRET_01_ARM_R_BONE].position.x = -armPosition;
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

        turret->flags |= TurretFlagsOpen;
        turret->stateTimer = 0.0f;
        turret->state = TurretStateGrabbed;
    }
}

static void turretCheckTipped(struct Turret* turret) {
    if (turret->rigidBody.flags & RigidBodyIsSleeping) {
        return;
    }

    struct Vector3 turretUp;
    quatMultVector(&turret->rigidBody.transform.rotation, &gUp, &turretUp);

    if (vector3Dot(&turretUp, &gUp) <= 0.0f) {
        turretPlaySound(
            turret,
            TurretSoundTypeDialog,
            RANDOM_TURRET_SOUND(sTurretTippedSounds),
            NPC_FLOORTURRET_TALKTIPPED
        );

        // Looped
        turret->currentSounds[TurretSoundTypeSfx] = soundPlayerPlay(
            SOUNDS_SHOOT1,
            5.0f,
            0.5f,
            &turret->rigidBody.transform.position,
            &turret->rigidBody.velocity,
            SoundTypeAll
        );

        turret->flags = (turret->flags & ~TurretFlagsRotating) | TurretFlagsOpen;
        turret->stateTimer = TURRET_TIPPED_DURATION;
        turret->state = TurretStateTipped;
    }
}

static void turretUpdateIdle(struct Turret* turret, struct Player* player) {
    turretCheckTipped(turret);
    turretCheckGrabbed(turret, player);
}

static void turretUpdateSearching(struct Turret* turret, struct Player* player) {
    struct SearchingStateData* state = &turret->stateData.searching;

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
        turretStartClose(
            turret,
            TurretStateIdle,
            RANDOM_TURRET_SOUND(sTurretRetireSounds),
            NPC_FLOORTURRET_TALKRETIRE
        );
    } else {
        turret->stateTimer -= FIXED_DELTA_TIME;
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
        turret->stateTimer = TURRET_SEARCH_DURATION;
        turret->stateData.searching.yawDirection = 1;
        turret->stateData.searching.pitchDirection = 1;
        turret->state = TurretStateSearching;
    }
}

static void turretUpdateTipped(struct Turret* turret) {
    if (!(turret->flags & TurretFlagsRotating)) {
        turretRotationFromYawPitch(mathfRandomFloat(), mathfRandomFloat(), &turret->targetRotation);
        turretStartRotation(turret, TURRET_TIPPED_ROTATE_SPEED);
    }

    if (turret->stateTimer <= 0.0f) {
        soundPlayerStop(turret->currentSounds[TurretSoundTypeSfx]);
        turretStartClose(
            turret,
            TurretStateDying,
            RANDOM_TURRET_SOUND(sTurretDisabledSounds),
            NPC_FLOORTURRET_TALKDISABLED
        );
    } else {
        turret->stateTimer -= FIXED_DELTA_TIME;
    }
}

static void turretUpdateClosing(struct Turret* turret, struct Player* player) {
    if (turret->stateTimer >= 0.0f) {
        turret->stateTimer -= FIXED_DELTA_TIME;
        return;
    }

    struct ClosingStateData* state = &turret->stateData.closing;

    if (turret->flags & TurretFlagsOpen) {
        turretPlaySound(
            turret,
            TurretSoundTypeDialog,
            state->soundId,
            state->subtitleId
        );

        turret->flags &= ~TurretFlagsOpen;
    } else if (turret->openAmount == 0.0f) {
        turret->state = state->nextState;
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

void turretUpdate(struct Turret* turret, struct Player* player) {
    if (turret->dynamicId == INVALID_DYNAMIC_OBJECT) {
        // TODO: delete, like decor
        return;
    }

    if (turret->collisionObject.flags & COLLISION_OBJECT_PLAYER_STANDING) {
        turret->collisionObject.flags &= ~COLLISION_OBJECT_PLAYER_STANDING;
    }
    dynamicSceneSetRoomFlags(turret->dynamicId, ROOM_FLAG_FROM_INDEX(turret->rigidBody.currentRoom));

    turretUpdateSounds(turret);

    if (turretUpdateFizzled(turret)) {
        return;
    }

    switch (turret->state) {
        case TurretStateIdle:
            turretUpdateIdle(turret, player);
            break;
        case TurretStateSearching:
            turretUpdateSearching(turret, player);
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
}
