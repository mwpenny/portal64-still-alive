#ifndef __SCENE_ELEVATOR_H__
#define __SCENE_ELEVATOR_H__

#include "../math/transform.h"
#include "../physics/collision_object.h"
#include "../player/player.h"
#include "../levels/level_definition.h"
#include "../scene/scene.h"

enum ElevatorFlags {
    ElevatorFlagsIsLocked = (1 << 0),
    ElevatorFlagsIsExit = (1 << 1),
    ElevatorFlagsHasHadPlayer = (1 << 2),
    ElevatorFlagsMovingSoundPlayed = (1 << 3),
};

struct Elevator {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    short dynamicId;
    short flags;
    short targetElevator;
    short roomIndex;
    float openAmount;
    float timer;
    float movingTimer;
};

void elevatorInit(struct Elevator* elevator, struct ElevatorDefinition* elevatorDefinition);

int elevatorUpdate(struct Elevator* elevator, struct Player* player);

#endif