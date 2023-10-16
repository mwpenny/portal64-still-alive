#ifndef __SCENE_BALL_H__
#define __SCENE_BALL_H__

#include "../physics/collision_object.h"

#define BALL_VELOCITY   3.0f
#define BALL_FADE_TIME  3.0f

#define MAX_BURN_MARKS  3

enum BallFlags {
    BallFlagsCaught = (1 << 0),
    BallFlagsPowering = (1 << 1),
    BallJustBounced = (1 << 2),
};

struct BallBurnMark {
    Mtx matrix;
    struct Vector3 at;
    struct Vector3 normal;
    short dynamicId;
};

struct Ball {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;

    float targetSpeed;
    float lifetime;
    float originalLifetime;
    short dynamicId;
    short flags;
    short soundLoopId;
};

void ballBurnMarkInit();
void ballBurnFilterOnPortal(struct Transform* portalTransform, int portalIndex);

void ballInitInactive(struct Ball* ball);
void ballInit(struct Ball* ball, struct Vector3* position, struct Vector3* velocity, short startingRoom, float ballLifetime);
void ballTurnOnCollision(struct Ball* ball);

void ballUpdate(struct Ball* ball);
int ballIsActive(struct Ball* ball);
int ballIsCollisionOn(struct Ball* ball);
int ballIsCaught(struct Ball* ball);
void ballMarkCaught(struct Ball* ball);

int isColliderForBall(struct CollisionObject* collisionObject);

#endif