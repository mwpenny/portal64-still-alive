#ifndef __DOOR_H__
#define __DOOR_H__

#include <stdint.h>

#include "audio/clips.h"
#include "audio/soundplayer.h"
#include "levels/level_definition.h"
#include "physics/collision_object.h"
#include "sk64/skeletool_animator.h"
#include "sk64/skeletool_armature.h"

struct Door {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    struct SKAnimator animator;
    struct SKArmature armature;

    struct Doorway* forDoorway;
    struct DoorDefinition* doorDefinition;
    short signalIndex;
    short dynamicId;
    uint8_t isOpen;
};

void doorInit(struct Door* door, struct DoorDefinition* doorDefinition, struct World* world);
void doorUpdate(struct Door* door);
void doorOnDeserialize(struct Door* door);

#endif
