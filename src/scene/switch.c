#include "switch.h"

#include "dynamic_scene.h"
#include "physics/collision_cylinder.h"
#include "physics/collision_scene.h"
#include "scene/hud.h"
#include "scene/scene.h"
#include "signals.h"
#include "util/dynamic_asset_loader.h"
#include "system/time.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/dynamic_animated_model_list.h"
#include "codegen/assets/models/props/switch001.h"

#define COLLIDER_HEIGHT                 0.7f

#define TICKTOCK_INITIAL_PAUSE_LENGTH   1.0f
#define TICKTOCK_PAUSE_LENGTH           0.25f
#define BUTTON_RAISE_LENGTH             0.5f

#define SWITCH_COLLISION_LAYERS (COLLISION_LAYERS_TANGIBLE | COLLISION_LAYERS_GRABBABLE | COLLISION_LAYERS_BLOCK_PORTAL | COLLISION_LAYERS_BLOCK_TURRET_SHOTS)

struct Vector2 gSwitchCylinderEdgeVectors[] = {
    {0.0f, 1.0f},
    {0.707f, 0.707f},
    {1.0f, 0.0f},
    {0.707f, -0.707f},
};

struct CollisionQuad gSwitchCylinderFaces[8];

struct CollisionCylinder gSwitchCylinder = {
    0.1f,
    COLLIDER_HEIGHT * 0.5f,
    gSwitchCylinderEdgeVectors,
    sizeof(gSwitchCylinderEdgeVectors) / sizeof(*gSwitchCylinderEdgeVectors),
    gSwitchCylinderFaces,
};

struct ColliderTypeData gSwitchCollider = {
    CollisionShapeTypeCylinder,
    &gSwitchCylinder,
    0.0f,
    1.0f,
    &gCollisionCylinderCallbacks
};

void switchRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct Switch* switchObj = (struct Switch*)data;

    struct Transform finalTransform = switchObj->rigidBody.transform;
    vector3AddScaled(&finalTransform.position, &gUp, -COLLIDER_HEIGHT * 0.5f, &finalTransform.position);

    Mtx* matrix = renderStateRequestMatrices(renderState, 1);

    if (!matrix) {
        return;
    }

    transformToMatrixL(&finalTransform, matrix, SCENE_SCALE);

    Mtx* armature = renderStateRequestMatrices(renderState, switchObj->armature.numberOfBones);

    if (!armature) {
        return;
    }

    skCalculateTransforms(&switchObj->armature, armature);

    dynamicRenderListAddData(
        renderList,
        switchObj->armature.displayList,
        matrix,
        BUTTON_INDEX,
        &switchObj->rigidBody.transform.position,
        armature
    );
}

void switchInit(struct Switch* switchObj, struct SwitchDefinition* definition) {
    struct SKArmatureWithAnimations* armature = dynamicAssetAnimatedModel(PROPS_SWITCH001_DYNAMIC_ANIMATED_MODEL);

    collisionObjectInit(&switchObj->collisionObject, &gSwitchCollider, &switchObj->rigidBody, 1.0f, SWITCH_COLLISION_LAYERS);
    rigidBodyMarkKinematic(&switchObj->rigidBody);
    collisionSceneAddDynamicObject(&switchObj->collisionObject);

    vector3AddScaled(&definition->location, &gUp, COLLIDER_HEIGHT * 0.5f, &switchObj->rigidBody.transform.position);
    switchObj->rigidBody.transform.rotation = definition->rotation;
    switchObj->rigidBody.transform.scale = gOneVec;
    switchObj->rigidBody.currentRoom = definition->roomIndex;

    collisionObjectUpdateBB(&switchObj->collisionObject);

    switchObj->dynamicId = dynamicSceneAdd(switchObj, switchRender, &switchObj->rigidBody.transform.position, COLLIDER_HEIGHT * 0.5f);
    switchObj->signalIndex = definition->signalIndex;

    dynamicSceneSetRoomFlags(switchObj->dynamicId, ROOM_FLAG_FROM_INDEX(switchObj->rigidBody.currentRoom));

    skAnimatorInit(&switchObj->animator, PROPS_SWITCH001_DEFAULT_BONES_COUNT);
    skArmatureInit(&switchObj->armature, armature->armature);

    switchObj->duration = definition->duration;
    switchObj->flags = 0;
    switchObj->timeLeft = 0.0f;
    switchObj->buttonRaiseTimer = 0.0f;
    switchObj->ticktockSoundLoopId = SOUND_ID_NONE;
    switchObj->ticktockPauseTimer = 0.0f;
}

void switchActivate(struct Switch* switchObj) {
    if ((switchObj->flags & SwitchFlagsDepressed) != 0 ||
        skAnimatorIsRunning(&switchObj->animator)) {
        return;
    }

    soundPlayerPlay(soundsButton, 1.0f, 1.0f, &switchObj->rigidBody.transform.position, &gZeroVec, SoundTypeAll);
    hudShowSubtitle(&gScene.hud, PORTAL_BUTTON_DOWN, SubtitleTypeCaption);

    // Wait longer for first tick sound
    switchObj->timeLeft = switchObj->duration;
    switchObj->ticktockPauseTimer = -(TICKTOCK_INITIAL_PAUSE_LENGTH - TICKTOCK_PAUSE_LENGTH);
    switchObj->ticktockSoundLoopId = SOUND_ID_NONE;

    switchObj->flags |= SwitchFlagsDepressed;
    skAnimatorRunClip(&switchObj->animator, dynamicAssetClip(PROPS_SWITCH001_DYNAMIC_ANIMATED_MODEL, PROPS_SWITCH001_ARMATURE_DOWN_CLIP_INDEX), 0.0f, 0);
}

void switchUpdate(struct Switch* switchObj) {
    skAnimatorUpdate(&switchObj->animator, switchObj->armature.pose, FIXED_DELTA_TIME);

    if (switchObj->collisionObject.flags & COLLISION_OBJECT_INTERACTED) {
        switchActivate(switchObj);
        switchObj->collisionObject.flags &= ~COLLISION_OBJECT_INTERACTED;
    }

    if ((switchObj->flags & SwitchFlagsDepressed) != 0 &&
        !skAnimatorIsRunning(&switchObj->animator)) {

        if (switchObj->buttonRaiseTimer < BUTTON_RAISE_LENGTH) {
            switchObj->buttonRaiseTimer += FIXED_DELTA_TIME;
        } else {
            switchObj->buttonRaiseTimer = 0.0f;
            switchObj->flags &= ~SwitchFlagsDepressed;
            skAnimatorRunClip(&switchObj->animator, dynamicAssetClip(PROPS_SWITCH001_DYNAMIC_ANIMATED_MODEL, PROPS_SWITCH001_ARMATURE_UP_CLIP_INDEX), 0.0f, 0);
        }
    }

    if (switchObj->timeLeft <= 0.0f) {
        return;
    }

    switchObj->timeLeft -= FIXED_DELTA_TIME;

    if (switchObj->timeLeft <= 0.0f) {
        switchObj->timeLeft = 0.0f;

        soundPlayerPlay(soundsButtonRelease, 1.0f, 1.0f, &switchObj->rigidBody.transform.position, &gZeroVec, SoundTypeAll);
        hudShowSubtitle(&gScene.hud, PORTAL_BUTTON_UP, SubtitleTypeCaption);
    } else {
        if (!soundPlayerIsPlaying(switchObj->ticktockSoundLoopId)){
            if (switchObj->ticktockPauseTimer < TICKTOCK_PAUSE_LENGTH) {
                switchObj->ticktockPauseTimer += FIXED_DELTA_TIME;
            } else {
                switchObj->ticktockPauseTimer = 0.0f;
                switchObj->ticktockSoundLoopId = soundPlayerPlay(soundsTickTock, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);
                hudShowSubtitle(&gScene.hud, PORTAL_ROOM1_TICKTOCK, SubtitleTypeCaption);
            }
        } else {
            switchObj->ticktockPauseTimer = 0.0f;
        }

        signalsSend(switchObj->signalIndex);
    }
}