#ifndef __DOOR_H__
#define __DOOR_H__

#include "../physics/collision_object.h"
#include "../levels/level_definition.h"
#include "../audio/soundplayer.h"
#include "../audio/clips.h"

enum DoorFlags {
    DoorFlagsJustClosed = (1 << 0),
    DoorFlagsJustOpened = (1 << 1),
};

struct Door {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    struct Doorway* forDoorway;
    struct DoorDefinition* doorDefinition;
    short dynamicId;
    short signalIndex;
    float openAmount;
    short flags;
};

void doorInit(struct Door* door, struct DoorDefinition* doorDefinition, struct World* world);
void doorUpdate(struct Door* door);

#endif