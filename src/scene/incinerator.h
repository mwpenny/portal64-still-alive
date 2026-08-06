#ifndef __INCINERATOR_H__
#define __INCINERATOR_H__

#include <stdint.h>

#include "levels/level_definition.h"
#include "sk64/skeletool_animator.h"
#include "sk64/skeletool_armature.h"

struct Incinerator {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    struct SKAnimator animator;
    struct SKArmature armature;
    float smokeTimer;
    short signalIndex;
    short dynamicId;
    uint8_t isOpen;
};

void incineratorInit(struct Incinerator* incinerator, struct IncineratorDefinition* definition);
void incineratorUpdate(struct Incinerator* incinerator);
void incineratorOnDeserialize(struct Incinerator* incinerator);

#endif
