#include "ball_launcher.h"

#include "../util/time.h"

#include "../physics/collision_box.h"
#include "../physics/collision_scene.h"
#include "dynamic_scene.h"
#include "signals.h"

#include "../build/assets/models/props/combine_ball_launcher.h"
#include "../build/assets/materials/static.h"

#define BALL_VELOCITY   2.0f

struct CollisionBox gBallLauncherBox = {
    {0.5f, 0.5f, 0.5f},
};

struct ColliderTypeData gBallLauncherCollider = {
    CollisionShapeTypeBox,
    &gBallLauncherBox,
    0.0f,
    1.0f,
    &gCollisionBoxCallbacks
};

void ballLauncherRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct BallLauncher* launcher = (struct BallLauncher*)data;

    Mtx* matrix = renderStateRequestMatrices(renderState, 1);

    if (!matrix) {
        return;
    }

    transformToMatrixL(&launcher->rigidBody.transform, matrix, SCENE_SCALE);

    Mtx* armature = renderStateRequestMatrices(renderState, launcher->armature.numberOfBones);

    if (!armature) {
        return;
    }

    skCalculateTransforms(&launcher->armature, armature);

    dynamicRenderListAddData(
        renderList,
        props_combine_ball_launcher_model_gfx,
        matrix,
        DEFAULT_INDEX,
        &launcher->rigidBody.transform.position,
        armature
    );
}

void ballLauncherInit(struct BallLauncher* launcher, struct BallLauncherDefinition* definition) {
    collisionObjectInit(&launcher->collisionObject, &gBallLauncherCollider, &launcher->rigidBody, 1.0f, COLLISION_LAYERS_TANGIBLE | COLLISION_LAYERS_BLOCK_BALL);
    rigidBodyMarkKinematic(&launcher->rigidBody);
    collisionSceneAddDynamicObject(&launcher->collisionObject);

    launcher->rigidBody.transform.position = definition->position;
    launcher->rigidBody.transform.rotation = definition->rotation;
    launcher->rigidBody.transform.scale = gOneVec;
    launcher->rigidBody.currentRoom = definition->roomIndex;

    launcher->signalIndex = definition->signalIndex;

    collisionObjectUpdateBB(&launcher->collisionObject);

    launcher->dynamicId = dynamicSceneAdd(launcher, ballLauncherRender, &launcher->rigidBody.transform, 1.0f);

    dynamicSceneSetRoomFlags(launcher->dynamicId, ROOM_FLAG_FROM_INDEX(launcher->rigidBody.currentRoom));

    skAnimatorInit(&launcher->animator, PROPS_COMBINE_BALL_LAUNCHER_DEFAULT_BONES_COUNT);
    skArmatureInit(&launcher->armature, &props_combine_ball_launcher_armature);

    ballInitInactive(&launcher->currentBall);
}

void ballLauncherUpdate(struct BallLauncher* launcher) {
    skAnimatorUpdate(&launcher->animator, launcher->armature.pose, FIXED_DELTA_TIME);

    ballUpdate(&launcher->currentBall);

    if (!ballIsActive(&launcher->currentBall) && signalsRead(launcher->signalIndex)) {
        struct Vector3 initialVelocity;
        quatMultVector(&launcher->rigidBody.transform.rotation, &gUp, &initialVelocity);
        vector3Scale(&initialVelocity, &initialVelocity, BALL_VELOCITY);

        ballInit(&launcher->currentBall, &launcher->rigidBody.transform.position, &initialVelocity, launcher->rigidBody.currentRoom);
    }
}