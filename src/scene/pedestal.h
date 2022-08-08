#ifndef __SCENE_PEDESTAL_H__
#define __SCENE_PEDESTAL_H__

#include "../sk64/skelatool_armature.h"
#include "../sk64/skelatool_animator.h"
#include "../levels/level_definition.h"

enum PedstalFlags {
    PedstalFlagsDown = (1 << 0),
};

struct Pedestal {
    struct Transform transform;
    struct SKArmature armature; 
    struct SKAnimator animator;

    short dynamicId;
    short roomIndex;

    short flags;
};

void pedestalInit(struct Pedestal* pedestal, struct PedestalDefinition* definition);
void pedestalUpdate(struct Pedestal* pedestal);

void pedestalHide(struct Pedestal* pedestal);

#endif