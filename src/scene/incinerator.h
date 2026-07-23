#ifndef __INCINERATOR_H__
#define __INCINERATOR_H__

#include <stdint.h>

#include "levels/level_definition.h"
#include "sk64/skeletool_animator.h"
#include "sk64/skeletool_armature.h"

struct Incinerator {
    struct SKAnimator animator;
    struct SKArmature armature;
    struct Transform transform;
    uint16_t signalIndex;
    uint8_t isOpen;
    uint8_t dynamicId;
};

void incineratorInit(struct Incinerator* incinerator, struct IncineratorDefinition* definition);
void incineratorUpdate(struct Incinerator* incinerator);

#endif
