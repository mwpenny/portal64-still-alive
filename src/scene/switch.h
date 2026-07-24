#ifndef __SWITCH_H__
#define __SWITCH_H__

#include <stdint.h>

#include "audio/clips.h"
#include "audio/soundplayer.h"
#include "levels/level_definition.h"
#include "physics/collision_object.h"
#include "sk64/skeletool_animator.h"
#include "sk64/skeletool_armature.h"

struct Switch {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    struct SKArmature armature;
    struct SKAnimator animator;

    float duration;
    short signalIndex;
    short dynamicId;

    float timeLeft;
    float buttonRaiseTimer;
    float ticktockPauseTimer;
    short ticktockSoundLoopId;
    uint8_t isDepressed;
};

void switchInit(struct Switch* switchObj, struct SwitchDefinition* definition);
void switchUpdate(struct Switch* switchObj);

#endif
