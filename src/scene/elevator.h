#ifndef __SCENE_ELEVATOR_H__
#define __SCENE_ELEVATOR_H__

#include "../math/transform.h"

struct Elevator {
    struct Transform transform;
    short dynamicId;
};

void elevatorInit(struct Elevator* elevator);

void elevatorUpdate(struct Elevator* elevator);

#endif