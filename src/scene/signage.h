#ifndef __SCENE_SIGNAGE_H__
#define __SCENE_SIGNAGE_H__

#include "../math/transform.h"
#include "../levels/level_definition.h"

struct Signage {
    struct Transform transform;
    short roomIndex;
    short testChamberNumber;
    float flickerChance;
};

void signageInit(struct Signage* signage, struct SignageDefinition* definition);

#endif