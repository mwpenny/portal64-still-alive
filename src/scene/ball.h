#ifndef __SCENE_BALL_H__
#define __SCENE_BALL_H__

#include "../physics/collision_object.h"

struct Ball {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    float targetSpeed;
    short dynamicId;
};

void ballInitInactive(struct Ball* ball);
void ballInit(struct Ball* ball, struct Vector3* position, struct Vector3* velocity, short startingRoom);

void ballUpdate(struct Ball* ball);
int ballIsActive(struct Ball* ball);

#endif