#ifndef __SCENE_BALL_CATCHER_H__
#define __SCENE_BALL_CATCHER_H__


#include "../sk64/skelatool_armature.h"
#include "../sk64/skelatool_animator.h"
#include "../physics/collision_object.h"

#include "../levels/level_definition.h"

#include "./ball_launcher.h"
#include "../audio/clips.h"
#include "../audio/soundplayer.h"

struct BallCatcher {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    struct SKArmature armature;
    struct SKAnimator animator;
    short dynamicId;
    short signalIndex;
    struct Ball* caughtBall;
};

void ballCatcherInit(struct BallCatcher* catcher, struct BallCatcherDefinition* definition);
void ballCatcherUpdate(struct BallCatcher* catcher, struct BallLauncher* ballLaunchers, int ballLauncherCount);

void ballCatcherHandBall(struct BallCatcher* catcher, struct Ball* caughtBall);

#endif