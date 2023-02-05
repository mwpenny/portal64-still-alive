#ifndef __SCENE_BALL_LAUNCHER_H__
#define __SCENE_BALL_LAUNCHER_H__

#include "../sk64/skelatool_armature.h"
#include "../sk64/skelatool_animator.h"
#include "../physics/collision_object.h"

#include "../levels/level_definition.h"

#include "./ball.h"

struct BallLauncher {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    struct SKArmature armature;
    struct SKAnimator animator;
    short dynamicId;
    short signalIndex;
    float ballLifetime;
    struct Ball currentBall;
};

void ballLauncherInit(struct BallLauncher* launcher, struct BallLauncherDefinition* definition);

void ballLauncherUpdate(struct BallLauncher* launcher);

#endif