#ifndef __DOOR_H__
#define __DOOR_H__

#include "../physics/collision_object.h"
#include "../levels/level_definition.h"
#include "../audio/soundplayer.h"
#include "../audio/clips.h"
#include "../sk64/skelatool_animator.h"
#include "../sk64/skelatool_armature.h"

enum DoorFlags {
    DoorFlagsIsOpen = (1 << 0),
};

struct DoorTypeDefinition {
    struct SKArmatureDefinition* armature;
    Gfx* model;
    struct SKAnimationClip* openClip;
    struct SKAnimationClip* closeClip;
    struct SKAnimationClip* openedClip;
    short materialIndex;
    short colliderBoneIndex;
    float closeSpeed;
    struct Quaternion relativeRotation;
};

struct Door {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    struct SKAnimator animator;
    struct SKArmature armature;

    struct Doorway* forDoorway;
    struct DoorDefinition* doorDefinition;
    short dynamicId;
    short signalIndex;
    short flags;
};

void doorInit(struct Door* door, struct DoorDefinition* doorDefinition, struct World* world);
void doorUpdate(struct Door* door);
void doorCheckForOpenState(struct Door* door);

#endif