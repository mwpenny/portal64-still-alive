#ifndef __SCENE_BALL_CATCHER_H__
#define __SCENE_BALL_CATCHER_H__


#include "../sk64/skelatool_armature.h"
#include "../sk64/skelatool_animator.h"
#include "../physics/collision_object.h"

#include "../levels/level_definition.h"

#include "./ball_launcher.h"

enum BallCatcherFlags {
    BallCatcherFlagsCaught = (1 << 0),
};

struct BallCatcher {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    struct SKArmature armature;
    struct SKAnimator animator;
    short dynamicId;
    short signalIndex;
    short flags;
};

void ballCatcherInit(struct BallCatcher* catcher, struct BallCatcherDefinition* definition);
void ballCatcherUpdate(struct BallCatcher* catcher, struct BallLauncher* ballLaunchers, int ballLauncherCount);

#endif