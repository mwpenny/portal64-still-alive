#ifndef __SCENE_SIGNAGE_H__
#define __SCENE_SIGNAGE_H__

#include "levels/level_definition.h"
#include "math/transform.h"

struct Signage {
    struct Transform transform;
    short roomIndex;
    short testChamberNumber;
    short currentFrame;
    short soundLoopId;
};

void signageInit(struct Signage* signage, struct SignageDefinition* definition);
void signageUpdate(struct Signage* signage);
void signageActivate(struct Signage* signage);

#endif