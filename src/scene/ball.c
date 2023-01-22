#include "./ball.h"

#include "dynamic_scene.h"
#include "../defs.h"

#include "../physics/collision_scene.h"
#include "../physics/collision_box.h"

#include "../build/assets/models/grav_flare.h"
#include "../build/assets/models/cube/cube.h"
#include "../build/assets/materials/static.h"

#define BALL_RADIUS 0.1f

struct CollisionBox gBallCollisionBox = {
    {BALL_RADIUS, BALL_RADIUS, BALL_RADIUS}
};

struct ColliderTypeData gBallCollider = {
    CollisionShapeTypeBox,
    &gBallCollisionBox,
    1.0f,
    0.0f,
    &gCollisionBoxCallbacks,
};

void ballRender(void* data, struct RenderScene* renderScene, struct Transform* fromView) {
    struct Ball* ball = (struct Ball*)data;
    struct Transform transform;
    transform.position = ball->rigidBody.transform.position;
    transform.rotation = fromView->rotation;
    vector3Scale(&gOneVec, &transform.scale, BALL_RADIUS);

    Mtx* mtx = renderStateRequestMatrices(renderScene->renderState, 1);

    transformToMatrixL(&transform, mtx, SCENE_SCALE);

    renderSceneAdd(renderScene, grav_flare_model_gfx, mtx, GRAV_FLARE_INDEX, &ball->rigidBody.transform.position, NULL);
}

void ballInitInactive(struct Ball* ball) {
    ball->targetSpeed = 0.0f;
    ball->flags = 0;
}

void ballInit(struct Ball* ball, struct Vector3* position, struct Vector3* velocity, short startingRoom) {
    collisionObjectInit(&ball->collisionObject, &gBallCollider, &ball->rigidBody, 1.0f, 0);

    collisionSceneAddDynamicObject(&ball->collisionObject);

    ball->rigidBody.flags |= RigidBodyDisableGravity;

    ball->rigidBody.velocity = *velocity;
    ball->rigidBody.transform.position = *position;
    quatIdent(&ball->rigidBody.transform.rotation);
    ball->rigidBody.transform.scale = gOneVec;
    ball->rigidBody.currentRoom = startingRoom;
    ball->flags = 0;

    ball->targetSpeed = sqrtf(vector3MagSqrd(&ball->rigidBody.velocity));

    ball->dynamicId = dynamicSceneAddViewDependant(ball, ballRender, &ball->rigidBody.transform, BALL_RADIUS);

    dynamicSceneSetRoomFlags(ball->dynamicId, ROOM_FLAG_FROM_INDEX(startingRoom));
}

void ballTurnOnCollision(struct Ball* ball) {
    ball->collisionObject.collisionLayers |= COLLISION_LAYERS_BLOCK_BALL;
}

void ballUpdate(struct Ball* ball) {
    if (ball->targetSpeed == 0.0f) {
        return;
    }

    float currentSpeed = sqrtf(vector3MagSqrd(&ball->rigidBody.velocity));

    if (currentSpeed == 0.0f) {
        vector3Scale(&gRight, &ball->rigidBody.velocity, ball->targetSpeed);
    } else {
        vector3Scale(&ball->rigidBody.velocity, &ball->rigidBody.velocity, ball->targetSpeed / currentSpeed);
    }

    ball->rigidBody.angularVelocity = gOneVec;
    
    dynamicSceneSetRoomFlags(ball->dynamicId, ROOM_FLAG_FROM_INDEX(ball->rigidBody.currentRoom));
}

int ballIsActive(struct Ball* ball) {
    return ball->targetSpeed != 0.0f;
}

int ballIsCollisionOn(struct Ball* ball) {
    return ball->collisionObject.collisionLayers != 0;
}

int ballIsCaught(struct Ball* ball) {
    return (ball->flags & BallFlagsCaught) != 0;
}

void ballMarkCaught(struct Ball* ball) {
    ball->flags |= BallFlagsCaught;
    rigidBodyMarkKinematic(&ball->rigidBody);
}