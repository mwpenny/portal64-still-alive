#ifndef __SCENE_CLOCK_H__
#define __SCENE_CLOCK_H__

#include "levels/level_definition.h"
#include "math/transform.h"

struct Clock {
    struct Transform transform;
    short roomIndex;
    float timeLeft;
};

void clockInit(struct Clock* clock, struct ClockDefinition* definition);
void clockShowMainMenuTime(struct Clock* clock);

void clockUpdate(struct Clock* clock);

#endif