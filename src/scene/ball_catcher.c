#include "ball_catcher.h"

#include "dynamic_scene.h"
#include "physics/collision_box.h"
#include "physics/collision_scene.h"
#include "signals.h"
#include "util/dynamic_asset_loader.h"
#include "util/frame_time.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/dynamic_animated_model_list.h"
#include "codegen/assets/models/props/combine_ball_catcher.h"

static struct CollisionBox sBallCatcherBox = {
    {0.5f, 0.5f, 0.5f},
};

static struct Vector3 sLocalCatcherLocation = {
    0.0f, 0.0f, -0.125f
};

static struct ColliderTypeData sBallCatcherCollider = {
    CollisionShapeTypeBox,
    &sBallCatcherBox,
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
    collisionObjectInit(&catcher->collisionObject, &sBallCatcherCollider, &catcher->rigidBody, 1.0f, COLLISION_LAYERS_TANGIBLE);
    rigidBodyMarkKinematic(&catcher->rigidBody);
    collisionSceneAddDynamicObject(&catcher->collisionObject);

    catcher->rigidBody.transform.position = definition->position;
    catcher->rigidBody.transform.rotation = definition->rotation;
    catcher->rigidBody.transform.scale = gOneVec;
    catcher->rigidBody.currentRoom = definition->roomIndex;
    collisionObjectUpdateBB(&catcher->collisionObject);

    catcher->caughtBall = NULL;
    catcher->signalIndex = definition->signalIndex;

    catcher->dynamicId = dynamicSceneAdd(catcher, ballCatcherRender, &catcher->rigidBody.transform.position, 1.0f);
    dynamicSceneSetRoomFlags(catcher->dynamicId, ROOM_FLAG_FROM_INDEX(catcher->rigidBody.currentRoom));

    struct SKArmatureWithAnimations* armature = dynamicAssetAnimatedModel(PROPS_COMBINE_BALL_CATCHER_DYNAMIC_ANIMATED_MODEL);
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
            &launcher->currentBall.rigidBody.velocity)
        ) {
            continue;
        }

        catcher->caughtBall = &launcher->currentBall;
        ballMarkCaught(catcher->caughtBall);

        soundPlayerPlay(soundsBallCatcher, 2.5f, 1.0f, &catcher->rigidBody.transform.position, &catcher->rigidBody.velocity, SoundTypeAll);
        skAnimatorRunClip(
            &catcher->animator,
            dynamicAssetClip(PROPS_COMBINE_BALL_CATCHER_DYNAMIC_ANIMATED_MODEL, PROPS_COMBINE_BALL_CATCHER_ARMATURE_CATCH_CLIP_INDEX),
            0.0f,
            0
        );
    }
}

void ballCatcherUpdate(struct BallCatcher* catcher, struct BallLauncher* ballLaunchers, int ballLauncherCount) {
    skAnimatorUpdate(&catcher->animator, catcher->armature.pose, FIXED_DELTA_TIME);

    if (catcher->caughtBall) {
        if (catcher->caughtBall->flags & BallFlagsPowering) {
            signalsSend(catcher->signalIndex);
        } else {
            struct Vector3 targetPosition;
            transformPoint(&catcher->rigidBody.transform, &sLocalCatcherLocation, &targetPosition);

            int reachedTarget = vector3MoveTowards(
                &catcher->caughtBall->rigidBody.transform.position,
                &targetPosition,
                FIXED_DELTA_TIME * BALL_VELOCITY,
                &catcher->caughtBall->rigidBody.transform.position
            );

            if (reachedTarget) {
                catcher->caughtBall->flags |= BallFlagsPowering;
                collisionSceneRemoveDynamicObject(&catcher->collisionObject);
            }
        }
    } else {
        ballCatcherCheckBalls(catcher, ballLaunchers, ballLauncherCount);
    }
}

void ballCatcherHandBall(struct BallCatcher* catcher, struct Ball* caughtBall) {
    catcher->caughtBall = caughtBall;
    catcher->caughtBall->flags |= BallFlagsPowering;
    ballMarkCaught(catcher->caughtBall);
    transformPoint(&catcher->rigidBody.transform, &sLocalCatcherLocation, &catcher->caughtBall->rigidBody.transform.position);

    skAnimatorRunClip(
        &catcher->animator,
        dynamicAssetClip(PROPS_COMBINE_BALL_CATCHER_DYNAMIC_ANIMATED_MODEL, PROPS_COMBINE_BALL_CATCHER_ARMATURE_CAUGHT_CLIP_INDEX),
        0.0f,
        0
    );
    collisionSceneRemoveDynamicObject(&catcher->collisionObject);
}
