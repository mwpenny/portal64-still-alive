#ifndef __SWITCH_H__
#define __SWITCH_H__

#include "audio/clips.h"
#include "audio/soundplayer.h"
#include "levels/level_definition.h"
#include "physics/collision_object.h"
#include "sk64/skeletool_animator.h"
#include "sk64/skeletool_armature.h"

enum SwitchFlags {
    SwitchFlagsDepressed = (1 << 0),
};

struct Switch {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    struct SKArmature armature;
    struct SKAnimator animator;
    short dynamicId;
    short signalIndex;
    short flags;
    float duration;
    float timeLeft;
    float buttonRaiseTimer;
    float ticktockPauseTimer;
    short ticktockSoundLoopId;
};

void switchInit(struct Switch* switchObj, struct SwitchDefinition* definition);

void switchUpdate(struct Switch* switchObj);

void switchActivate(struct Switch* switchObj);

#endif