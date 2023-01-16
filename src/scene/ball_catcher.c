#include "ball_catcher.h"

#include "../util/time.h"

#include "../physics/collision_box.h"
#include "../physics/collision_scene.h"
#include "dynamic_scene.h"
#include "signals.h"

#include "../build/assets/models/props/combine_ball_catcher.h"
#include "../build/assets/materials/static.h"

struct CollisionBox gBallCatcherBox = {
    {0.5f, 0.5f, 0.5f},
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
        props_combine_ball_catcher_model_gfx,
        matrix,
        DEFAULT_INDEX,
        &catcher->rigidBody.transform.position,
        armature
    );
}

void ballCatcherInit(struct BallCatcher* catcher, struct BallCatcherDefinition* definition) {
    collisionObjectInit(&catcher->collisionObject, &gBallCatcherCollider, &catcher->rigidBody, 1.0f, COLLISION_LAYERS_TANGIBLE);
    rigidBodyMarkKinematic(&catcher->rigidBody);
    collisionSceneAddDynamicObject(&catcher->collisionObject);

    catcher->rigidBody.transform.position = definition->position;
    catcher->rigidBody.transform.rotation = definition->rotation;
    catcher->rigidBody.transform.scale = gOneVec;
    catcher->rigidBody.currentRoom = definition->roomIndex;

    catcher->signalIndex = definition->signalIndex;

    collisionObjectUpdateBB(&catcher->collisionObject);

    catcher->dynamicId = dynamicSceneAdd(catcher, ballCatcherRender, &catcher->rigidBody.transform, 1.0f);

    dynamicSceneSetRoomFlags(catcher->dynamicId, ROOM_FLAG_FROM_INDEX(catcher->rigidBody.currentRoom));

    skAnimatorInit(&catcher->animator, PROPS_COMBINE_BALL_CATCHER_DEFAULT_BONES_COUNT);
    skArmatureInit(&catcher->armature, &props_combine_ball_catcher_armature);
}

void ballCatcherUpdate(struct BallCatcher* catcher) {
    skAnimatorUpdate(&catcher->animator, catcher->armature.pose, FIXED_DELTA_TIME);
}