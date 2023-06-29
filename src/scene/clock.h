#ifndef __SCENE_CLOCK_H__
#define __SCENE_CLOCK_H__

#include "../math/transform.h"
#include "../levels/level_definition.h"

struct Clock {
    struct Transform transform;
    short roomIndex;
    short cutsceneIndex;
};

void clockInit(struct Clock* clock, struct ClockDefinition* definition);

void clockUpdate(struct Clock* clock);

#endif