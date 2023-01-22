#ifndef __SCENE_BALL_H__
#define __SCENE_BALL_H__

#include "../physics/collision_object.h"

#define BALL_VELOCITY   2.0f
#define BALL_LIFETIME   10.0f
#define BALL_FADE_TIME  3.0f

enum BallFlags {
    BallFlagsCaught = (1 << 0),
    BallFlagsPowering = (1 << 1),
};

struct Ball {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    float targetSpeed;
    float lifetime;
    short dynamicId;
    short flags;
};

void ballInitInactive(struct Ball* ball);
void ballInit(struct Ball* ball, struct Vector3* position, struct Vector3* velocity, short startingRoom);
void ballTurnOnCollision(struct Ball* ball);

void ballUpdate(struct Ball* ball);
int ballIsActive(struct Ball* ball);
int ballIsCollisionOn(struct Ball* ball);
int ballIsCaught(struct Ball* ball);
void ballMarkCaught(struct Ball* ball);

#endif