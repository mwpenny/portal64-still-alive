#include "turret.h"

#include "decor/decor_object.h"
#include "defs.h"
#include "locales/locales.h"
#include "physics/collision_scene.h"
#include "savefile/savefile.h"
#include "scene/dynamic_scene.h"
#include "util/dynamic_asset_loader.h"

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

static struct Vector3 gTurretOriginOffset = { 0.0f, 0.554f, 0.0f };
static struct Vector3 gTurretLaserOffset = { 0.0f, 0.0425f, 0.11f };

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

#define TURRET_MASS               3.0f
#define TURRET_COLLISION_LAYERS   (COLLISION_LAYERS_TANGIBLE | COLLISION_LAYERS_GRABBABLE | COLLISION_LAYERS_FIZZLER | COLLISION_LAYERS_BLOCK_TURRET_SHOTS)

#define TURRET_OPEN_POSITION      46.0f
#define TURRET_CLOSE_POSITION     22.0f
#define TURRET_OPEN_SPEED         4.0f
#define TURRET_CLOSE_SPEED        2.0f

#define TURRET_ROTATE_SPEED       2.0f
#define TURRET_MIN_ROTATION_DELAY 0.1f
#define TURRET_MAX_ROTATION_DELAY 0.75f

#define TURRET_LOOK_RANGE_X       0.4f
#define TURRET_LOOK_RANGE_Y       0.5f
#define TURRET_LOOK_Z             1.0f

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

static void turretRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct Turret* turret = (struct Turret*)data;

    Mtx* matrix = renderStateRequestMatrices(renderState, 1);

    if (!matrix) {
        return;
    }

    // Render without origin offset
    struct Vector3 transformedOriginOffset;
    quatMultVector(&turret->rigidBody.transform.rotation, &gTurretOriginOffset, &transformedOriginOffset);

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
    quatMultVector(&turret->rigidBody.transform.rotation, &gTurretOriginOffset, &originOffset);
    vector3Add(&definition->position, &originOffset, &turret->rigidBody.transform.position);

    collisionObjectUpdateBB(&turret->collisionObject);

    struct SKArmatureWithAnimations* armature = dynamicAssetAnimatedModel(PROPS_TURRET_01_DYNAMIC_ANIMATED_MODEL);
    skArmatureInit(&turret->armature, armature->armature);

    laserInit(
        &turret->laser,
        &turret->rigidBody,
        &gTurretLaserOffset,
        &turret->armature.pose[PROPS_TURRET_01_ARM_C_BONE].rotation
    );

    turret->dynamicId = dynamicSceneAdd(turret, turretRender, &turret->rigidBody.transform.position, 0.75f);
    dynamicSceneSetRoomFlags(turret->dynamicId, ROOM_FLAG_FROM_INDEX(turret->rigidBody.currentRoom));

    quatIdent(&turret->targetRotation);

    turret->fizzleTime = 0.0f;
    turret->state = TurretStateIdle;
    turret->flags = 0;
    turret->openAmount = 0.0f;
    turret->rotationDelayTimer = 0.0f;

    for (enum TurretSoundType t = 0; t < TurretSoundTypeCount; ++t) {
        turret->currentSounds[t] = SOUND_ID_NONE;
    }
}

// State handler functions

static void turretUpdateSounds(struct Turret* turret) {
    for (enum TurretSoundType t = 0; t < TurretSoundTypeCount; ++t) {
        SoundId soundId = turret->currentSounds[t];
        if (soundId == SOUND_ID_NONE) {
            continue;
        }

        soundPlayerUpdatePosition(
            soundId,
            &turret->rigidBody.transform.position,
            &turret->rigidBody.velocity
        );

        if (!soundPlayerIsPlaying(soundId)) {
            turret->currentSounds[t] = SOUND_ID_NONE;
        }
    }
}

static uint8_t turretUpdateFizzled(struct Turret* turret) {
    enum FizzleCheckResult fizzleStatus = decorObjectUpdateFizzler(&turret->collisionObject, &turret->fizzleTime);
    if (fizzleStatus == FizzleCheckResultStart) {
        laserRemove(&turret->laser);

        turretPlaySound(
            turret,
            TurretSoundTypeDialog,
            SOUNDS_TURRET_FIZZLER_1,
            NPC_FLOORTURRET_TALKDISSOLVED
        );
    } else if (fizzleStatus == FizzleCheckResultEnd && !soundPlayerIsPlaying(turret->currentSounds[TurretSoundTypeDialog])) {
        dynamicSceneRemove(turret->dynamicId);
        collisionSceneRemoveDynamicObject(&turret->collisionObject);
        turret->dynamicId = INVALID_DYNAMIC_OBJECT;
    }

    return (turret->rigidBody.flags & RigidBodyFizzled) != 0;
}

static void turretUpdateRotation(struct Turret* turret) {
    struct Quaternion* currentRotation = &turret->armature.pose[PROPS_TURRET_01_ARM_C_BONE].rotation;

    if (quatRotateTowards(
        currentRotation,
        &turret->targetRotation,
        TURRET_ROTATE_SPEED * FIXED_DELTA_TIME,
        currentRotation
    )) {
        turret->flags &= ~TurretFlagsRotating;
    }
}

static void turretSetOpen(struct Turret* turret, uint8_t shouldOpen) {
    if (turret->openAmount == shouldOpen) {
        return;
    }

    short soundId = SOUND_ID_NONE;
    short subtitleId = StringIdNone;
    if (turret->openAmount == 0.0f && shouldOpen) {
        soundId = SOUNDS_DEPLOY;
        subtitleId = NPC_FLOORTURRET_DEPLOY;
    } else if (turret->openAmount == 1.0f && !shouldOpen) {
        soundId = SOUNDS_RETRACT;
        subtitleId = NPC_FLOORTURRET_RETRACT;
    }

    if (soundId != SOUND_ID_NONE) {
        // Don't use turretPlaySound(), to allow others to play over top of this
        soundPlayerPlay(
            soundId,
            1.0f,
            0.5f,
            &turret->rigidBody.transform.position,
            &turret->rigidBody.velocity,
            SoundTypeAll
        );
        hudShowSubtitle(&gScene.hud, subtitleId, SubtitleTypeCaption);
    }

    float speed = shouldOpen ? TURRET_OPEN_SPEED : -TURRET_CLOSE_SPEED;
    turret->openAmount = clampf(turret->openAmount + (speed * FIXED_DELTA_TIME), 0.0f, 1.0f);

    float armPosition = mathfLerp(TURRET_CLOSE_POSITION, TURRET_OPEN_POSITION, turret->openAmount);
    turret->armature.pose[PROPS_TURRET_01_ARM_L_BONE].position.x = armPosition;
    turret->armature.pose[PROPS_TURRET_01_ARM_R_BONE].position.x = -armPosition;
}

static void turretSetLookOffset(struct Turret* turret, struct Vector3* lookOffset) {
    if (lookOffset == NULL) {
        quatIdent(&turret->targetRotation);
    } else {
        struct Vector3 finalOffset;
        vector3Negate(lookOffset, &finalOffset);
        quatLook(&finalOffset, &gUp, &turret->targetRotation);
    }

    turret->flags |= TurretFlagsRotating;
}

static void turretUpdateIdle(struct Turret* turret, struct Player* player) {
    turretSetOpen(turret, 0);

    if (playerIsGrabbingObject(player, &turret->collisionObject)) {
        turretPlaySound(
            turret,
            TurretSoundTypeDialog,
            RANDOM_TURRET_SOUND(sTurretPickupSounds),
            NPC_FLOORTURRET_TALKPICKUP
        );
        hudShowSubtitle(&gScene.hud, NPC_FLOORTURRET_ACTIVATE, SubtitleTypeCaption);

        turret->rotationDelayTimer = 0.0f;
        turret->state = TurretStateGrabbed;
    }
}

static void turretUpdateSearching(struct Turret* turret) {
    // TODO

    turretPlaySound(
        turret,
        TurretSoundTypeDialog,
        RANDOM_TURRET_SOUND(sTurretRetireSounds),
        NPC_FLOORTURRET_TALKRETIRE
    );

    turret->state = TurretStateIdle;
}

static void turretUpdateGrabbed(struct Turret* turret, struct Player* player) {
    turretSetOpen(turret, 1);

    // Look in a random direction, pause, repeat
    if (!(turret->flags & TurretFlagsRotating)) {
        if (turret->rotationDelayTimer <= 0.0f) {
            struct Vector3 lookTarget = {
                randomInRangef(-TURRET_LOOK_RANGE_X, TURRET_LOOK_RANGE_X),
                randomInRangef(-TURRET_LOOK_RANGE_Y, TURRET_LOOK_RANGE_Y),
                TURRET_LOOK_Z
            };
            turretSetLookOffset(turret, &lookTarget);

            turretPlaySound(
                turret,
                TurretSoundTypeSfx,
                SOUNDS_ACTIVE,
                StringIdNone  // Don't spam
            );

            turret->rotationDelayTimer = randomInRangef(
                TURRET_MIN_ROTATION_DELAY,
                TURRET_MAX_ROTATION_DELAY
            );
        } else {
            turret->rotationDelayTimer -= FIXED_DELTA_TIME;
        }
    }

    if (!playerIsGrabbingObject(player, &turret->collisionObject)) {
        turretSetLookOffset(turret, NULL);

        turret->state = TurretStateSearching;
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
        case TurretStateSearching:
            turretUpdateSearching(turret);
            break;
        case TurretStateGrabbed:
            turretUpdateGrabbed(turret, player);
            break;
        default:
        case TurretStateIdle:
            turretUpdateIdle(turret, player);
            break;
    }

    if (turret->flags & TurretFlagsRotating) {
        turretUpdateRotation(turret);
    }
}
