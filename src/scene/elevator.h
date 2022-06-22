#ifndef __SCENE_ELEVATOR_H__
#define __SCENE_ELEVATOR_H__

#include "../math/transform.h"
#include "../physics/collision_object.h"
#include "../player/player.h"

enum ElevatorFlags {
    ElevatorFlagsIsOpen = (1 << 0),
    ElevatorFlagsContainsPlayer = (1 << 1),
    ElevatorFlagsReleasePlayer = (1 << 2),
    ElevatorFlagsIsLocked = (1 << 3),
};

struct Elevator {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    short dynamicId;
    short flags;
};

void elevatorInit(struct Elevator* elevator);

void elevatorUpdate(struct Elevator* elevator, struct Player* player);

#endif