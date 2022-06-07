#ifndef __DOOR_H__
#define __DOOR_H__

#include "../physics/collision_object.h"
#include "../levels/level_definition.h"

struct Door {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    struct Doorway* forDoorway;
    struct DoorDefinition* doorDefinition;
    short dynamicId;
    short signalIndex;
    float openAmount;
};

void doorInit(struct Door* door, struct DoorDefinition* doorDefinition, struct World* world);
void doorUpdate(struct Door* door);

#endif