#ifndef __SWITCH_H__
#define __SWITCH_H__

#include "../sk64/skelatool_armature.h"
#include "../sk64/skelatool_animator.h"
#include "../physics/collision_object.h"
#include "../audio/soundplayer.h"
#include "../audio/clips.h"

#include "../levels/level_definition.h"

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
    short ticktockSoundLoopId;
    float ticktockPauseTimer;
};

void switchInit(struct Switch* switchObj, struct SwitchDefinition* definition);

void switchUpdate(struct Switch* switchObj);

void switchActivate(struct Switch* switchObj);

#endif