#ifndef __SCENE_PEDESTAL_H__
#define __SCENE_PEDESTAL_H__

#include "../sk64/skelatool_armature.h"
#include "../levels/level_definition.h"

struct Pedestal {
    struct Transform transform;
    struct SKArmature armature; 

    short dynamicId;
    short roomIndex;
};

void pedestalInit(struct Pedestal* pedestal, struct PedestalDefinition* definition);

#endif