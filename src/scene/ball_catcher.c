#include "ball_catcher.h"

#include "dynamic_scene.h"
#include "physics/collision_box.h"
#include "physics/collision_scene.h"
#include "signals.h"
#include "system/time.h"
#include "util/dynamic_asset_loader.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/dynamic_animated_model_list.h"
#include "codegen/assets/models/props/combine_ball_catcher.h"

struct CollisionBox gBallCatcherBox = {
    {0.5f, 0.5f, 0.5f},
};

struct Vector3 gLocalCatcherLocation = {
    0.0f, 0.0f, -0.125f
};

struct ColliderTypeData gBallCatcherCollider = {
    CollisionShapeTypeBox,
    &gBallCatcherBox,
    0.0f,
    1.0f,
    &gCollisionBoxCallbacks
};

void ballCatcherRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct BallCatcher* catcher = (struct BallCatcher*)data;

    Mtx* matrix = renderStateRequestMatrices(renderState, 1);

    if (!matrix) {
        return;
    }

    transformToMatrixL(&catcher->rigidBody.transform, matrix, SCENE_SCALE);

    Mtx* armature = renderStateRequestMatrices(renderState, catcher->armature.numberOfBones);

    if (!armature) {
        return;
    }

    skCalculateTransforms(&catcher->armature, armature);

    dynamicRenderListAddData(
        renderList,
        catcher->armature.displayList,
        matrix,
        BALL_CATCHER_INDEX,
        &catcher->rigidBody.transform.position,
        armature
    );
}

void ballCatcherInit(struct BallCatcher* catcher, struct BallCatcherDefinition* definition) {
    struct SKArmatureWithAnimations* armature = dynamicAssetAnimatedModel(PROPS_COMBINE_BALL_CATCHER_DYNAMIC_ANIMATED_MODEL);

    collisionObjectInit(&catcher->collisionObject, &gBallCatcherCollider, &catcher->rigidBody, 1.0f, COLLISION_LAYERS_TANGIBLE);
    rigidBodyMarkKinematic(&catcher->rigidBody); 
    collisionSceneAddDynamicObject(&catcher->collisionObject);

    catcher->rigidBody.transform.position = definition->position;
    catcher->rigidBody.transform.rotation = definition->rotation;
    catcher->rigidBody.transform.scale = gOneVec;
    catcher->rigidBody.currentRoom = definition->roomIndex;

    catcher->signalIndex = definition->signalIndex;

    catcher->caughtBall = NULL;

    collisionObjectUpdateBB(&catcher->collisionObject);

    catcher->dynamicId = dynamicSceneAdd(catcher, ballCatcherRender, &catcher->rigidBody.transform.position, 1.0f);

    dynamicSceneSetRoomFlags(catcher->dynamicId, ROOM_FLAG_FROM_INDEX(catcher->rigidBody.currentRoom));

    skAnimatorInit(&catcher->animator, PROPS_COMBINE_BALL_CATCHER_DEFAULT_BONES_COUNT);
    skArmatureInit(&catcher->armature, armature->armature);
}

void ballCatcherCheckBalls(struct BallCatcher* catcher, struct BallLauncher* ballLaunchers, int ballLauncherCount) {
    if (catcher->caughtBall) {
        return;
    }

    for (int i = 0; i < ballLauncherCount; ++i) {
        struct BallLauncher* launcher = &ballLaunchers[i];

        if (!ballIsActive(&launcher->currentBall) || ballIsCaught(&launcher->currentBall)) {
            continue;
        }

        struct Simplex simplex;
        if (!gjkCheckForOverlap(
            &simplex,
            &catcher->collisionObject,
            objectMinkowskiSupport,
            &launcher->currentBall.collisionObject,
            objectMinkowskiSupport,
            &launcher->currentBall.rigidBody.velocity)) {
            continue;
        }

        catcher->caughtBall = &launcher->currentBall;
        soundPlayerPlay(soundsBallCatcher, 2.5f, 0.5f, &catcher->rigidBody.transform.position, &catcher->rigidBody.velocity, SoundTypeAll);
        ballMarkCaught(catcher->caughtBall);
        skAnimatorRunClip(&catcher->animator, dynamicAssetClip(PROPS_COMBINE_BALL_CATCHER_DYNAMIC_ANIMATED_MODEL, PROPS_COMBINE_BALL_CATCHER_ARMATURE_CATCH_CLIP_INDEX), 0.0f, 0);
    }
}

void ballCatcherUpdate(struct BallCatcher* catcher, struct BallLauncher* ballLaunchers, int ballLauncherCount) {
    skAnimatorUpdate(&catcher->animator, catcher->armature.pose, FIXED_DELTA_TIME);
    
    if (catcher->caughtBall) {
        if (catcher->caughtBall->flags & BallFlagsPowering) {
            signalsSend(catcher->signalIndex);
        } else {
            struct Vector3 targetPosition;
            transformPoint(&catcher->rigidBody.transform, &gLocalCatcherLocation, &targetPosition);

            vector3MoveTowards(
                &catcher->caughtBall->rigidBody.transform.position, 
                &targetPosition, 
                FIXED_DELTA_TIME * BALL_VELOCITY, 
                &catcher->caughtBall->rigidBody.transform.position
            );

            if (targetPosition.x == catcher->caughtBall->rigidBody.transform.position.x && 
                targetPosition.y == catcher->caughtBall->rigidBody.transform.position.y && 
                targetPosition.z == catcher->caughtBall->rigidBody.transform.position.z) {
                catcher->caughtBall->flags |= BallFlagsPowering;
            }
        }
    } else {
        ballCatcherCheckBalls(catcher, ballLaunchers, ballLauncherCount);
    }
}

void ballCatcherHandBall(struct BallCatcher* catcher, struct Ball* caughtBall) {
    catcher->caughtBall = caughtBall;
    struct Vector3 targetPosition;
    transformPoint(&catcher->rigidBody.transform, &gLocalCatcherLocation, &targetPosition);
    catcher->caughtBall->rigidBody.transform.position = targetPosition;
    catcher->caughtBall->flags |= BallFlagsPowering;
}