#ifndef __DOOR_H__
#define __DOOR_H__

#include "../physics/collision_object.h"

struct Door {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    short dynamicId;
    short signalIndex;
    float openAmount;
};

void doorInit(struct Door* door, struct Transform* at, int roomA, int roomB, int doorwayIndex, int signalIndex);
void doorUpdate(struct Door* door);

#endif