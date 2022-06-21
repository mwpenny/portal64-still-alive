#ifndef __SCENE_ELEVATOR_H__
#define __SCENE_ELEVATOR_H__

#include "../math/transform.h"
#include "../physics/collision_object.h"

struct Elevator {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    short dynamicId;
};

void elevatorInit(struct Elevator* elevator);

void elevatorUpdate(struct Elevator* elevator);

#endif