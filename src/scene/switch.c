#include "switch.h"

#include "../physics/collision_cylinder.h"
#include "../physics/collision_scene.h"
#include "dynamic_scene.h"
#include "signals.h"

#include "../build/assets/models/props/switch001.h"
#include "../build/assets/materials/static.h"

#include "../util/time.h"

#define COLLIDER_HEIGHT   0.7f
#define TICKTOCK_PAUSE_LENGTH  0.25f

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
        props_switch001_model_gfx,
        matrix,
        BUTTON_INDEX,
        &switchObj->rigidBody.transform.position,
        armature
    );
}

void switchInit(struct Switch* switchObj, struct SwitchDefinition* definition) {
    collisionObjectInit(&switchObj->collisionObject, &gSwitchCollider, &switchObj->rigidBody, 1.0f, COLLISION_LAYERS_TANGIBLE);
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
    skArmatureInit(&switchObj->armature, &props_switch001_armature);

    switchObj->duration = definition->duration;
    switchObj->flags = 0;
    switchObj->timeLeft = 0.0f;
    switchObj->ticktockPauseTimer = 0.0f;
}

void switchActivate(struct Switch* switchObj) {
    if (switchObj->timeLeft > 0.0f) {
        return;
    }
    soundPlayerPlay(soundsButton, 1.0f, 0.5f, &switchObj->rigidBody.transform.position, &gZeroVec);
    switchObj->ticktockSoundLoopId = soundPlayerPlay(soundsTickTock, 1.0f, 0.5f, NULL, NULL);
    switchObj->flags |= SwitchFlagsDepressed;
    switchObj->timeLeft = switchObj->duration;
    signalsSend(switchObj->signalIndex);
    skAnimatorRunClip(&switchObj->animator, &props_switch001_Armature_down_clip, 0.0f, 0);
}

void switchUpdate(struct Switch* switchObj) {
    skAnimatorUpdate(&switchObj->animator, switchObj->armature.pose, FIXED_DELTA_TIME);

    if (switchObj->collisionObject.flags & COLLISION_OBJECT_INTERACTED) {
        switchActivate(switchObj);
        switchObj->collisionObject.flags &= ~COLLISION_OBJECT_INTERACTED;
    }   

    if (switchObj->timeLeft <= 0.0f) {
        if ((switchObj->flags & SwitchFlagsDepressed) != 0 && 
            !skAnimatorIsRunning(&switchObj->animator)) {
            switchObj->flags &= ~SwitchFlagsDepressed;
            skAnimatorRunClip(&switchObj->animator, &props_switch001_Armature_up_clip, 0.0f, 0);
        }

        return;
    }

    switchObj->timeLeft -= FIXED_DELTA_TIME;


    

    if (switchObj->timeLeft < 0.0f) {
        switchObj->timeLeft = 0.0f;
    } else {
        if (!soundPlayerIsPlaying(switchObj->ticktockSoundLoopId)){
            if (switchObj->ticktockPauseTimer < TICKTOCK_PAUSE_LENGTH){
                switchObj->ticktockPauseTimer += FIXED_DELTA_TIME;
            }else{
                switchObj->ticktockPauseTimer = 0; 
                switchObj->ticktockSoundLoopId = soundPlayerPlay(soundsTickTock, 1.0f, 0.5f, NULL, NULL);
            }
        }else{
            switchObj->ticktockPauseTimer = 0; 
        }

        signalsSend(switchObj->signalIndex);
    }
}