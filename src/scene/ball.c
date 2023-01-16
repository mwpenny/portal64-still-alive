#include "./ball.h"

#include "dynamic_scene.h"

#define BALL_RADIUS 0.1f

void ballRender(void* data, struct RenderScene* renderList, struct Transform* fromView) {

}

void ballInitInactive(struct Ball* ball) {
    ball->targetSpeed = 0.0f;
}

void ballInit(struct Ball* ball, struct Vector3* position, struct Vector3* velocity, short startingRoom) {
    ball->rigidBody.velocity = *velocity;
    ball->rigidBody.transform.position = *position;
    ball->rigidBody.currentRoom = startingRoom;

    ball->targetSpeed = sqrtf(vector3MagSqrd(&ball->rigidBody.velocity));

    ball->dynamicId = dynamicSceneAddViewDependant(ball, ballRender, &ball->rigidBody.transform, BALL_RADIUS);

    dynamicSceneSetRoomFlags(ball->dynamicId, ROOM_FLAG_FROM_INDEX(startingRoom));
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