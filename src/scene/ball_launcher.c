#include "ball_launcher.h"

#include "audio/clips.h"
#include "audio/soundplayer.h"
#include "dynamic_scene.h"
#include "physics/collision_box.h"
#include "physics/collision_scene.h"
#include "signals.h"
#include "system/time.h"
#include "util/dynamic_asset_loader.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/dynamic_animated_model_list.h"
#include "codegen/assets/models/props/combine_ball_launcher.h"

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
        launcher->armature.displayList,
        matrix,
        BALL_CATCHER_INDEX,
        &launcher->rigidBody.transform.position,
        armature
    );
}

void ballLauncherInit(struct BallLauncher* launcher, struct BallLauncherDefinition* definition) {
    struct SKArmatureWithAnimations* armature = dynamicAssetAnimatedModel(PROPS_COMBINE_BALL_LAUNCHER_DYNAMIC_ANIMATED_MODEL);

    collisionObjectInit(&launcher->collisionObject, &gBallLauncherCollider, &launcher->rigidBody, 1.0f, COLLISION_LAYERS_TANGIBLE | COLLISION_LAYERS_BLOCK_BALL);
    rigidBodyMarkKinematic(&launcher->rigidBody);
    collisionSceneAddDynamicObject(&launcher->collisionObject);

    launcher->rigidBody.transform.position = definition->position;
    launcher->rigidBody.transform.rotation = definition->rotation;
    launcher->rigidBody.transform.scale = gOneVec;
    launcher->rigidBody.currentRoom = definition->roomIndex;

    launcher->signalIndex = definition->signalIndex;
    launcher->ballLifetime = definition->ballLifetime;
    launcher->ballVelocity = definition->ballVelocity;

    collisionObjectUpdateBB(&launcher->collisionObject);

    launcher->dynamicId = dynamicSceneAdd(launcher, ballLauncherRender, &launcher->rigidBody.transform.position, 1.0f);

    dynamicSceneSetRoomFlags(launcher->dynamicId, ROOM_FLAG_FROM_INDEX(launcher->rigidBody.currentRoom));

    skAnimatorInit(&launcher->animator, PROPS_COMBINE_BALL_LAUNCHER_DEFAULT_BONES_COUNT);
    skArmatureInit(&launcher->armature, armature->armature);

    ballInitInactive(&launcher->currentBall);
}

void ballLauncherUpdate(struct BallLauncher* launcher) {
    skAnimatorUpdate(&launcher->animator, launcher->armature.pose, FIXED_DELTA_TIME);

    if (ballIsCaught(&launcher->currentBall)) {
        return;
    }

    ballUpdate(&launcher->currentBall);

    if (!ballIsActive(&launcher->currentBall) && signalsRead(launcher->signalIndex)) {
        struct Vector3 initialVelocity;
        quatMultVector(&launcher->rigidBody.transform.rotation, &gForward, &initialVelocity);
        vector3Scale(&initialVelocity, &initialVelocity, launcher->ballVelocity);

        ballInit(&launcher->currentBall, &launcher->rigidBody.transform.position, &initialVelocity, launcher->rigidBody.currentRoom, launcher->ballLifetime);
        skAnimatorRunClip(&launcher->animator, dynamicAssetClip(PROPS_COMBINE_BALL_LAUNCHER_DYNAMIC_ANIMATED_MODEL, PROPS_COMBINE_BALL_LAUNCHER_ARMATURE_LAUNCH_CLIP_INDEX), 0.0f, 0);
        soundPlayerPlay(soundsBallLaunch, 1.0f, 0.5f, &launcher->rigidBody.transform.position, &gZeroVec, SoundTypeAll);
    }

    if (ballIsActive(&launcher->currentBall) && !ballIsCollisionOn(&launcher->currentBall)) {
        struct Simplex simplex;
        if (!gjkCheckForOverlap(
            &simplex, 
            &launcher->collisionObject, 
            objectMinkowskiSupport,
            &launcher->currentBall.collisionObject,
            objectMinkowskiSupport,
            &launcher->currentBall.rigidBody.velocity)) {
            ballTurnOnCollision(&launcher->currentBall);
        }
    }
}