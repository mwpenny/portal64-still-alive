#ifndef __SCENE_BALL_LAUNCHER_H__
#define __SCENE_BALL_LAUNCHER_H__

#include "ball.h"

#include "levels/level_definition.h"
#include "physics/collision_object.h"
#include "sk64/skeletool_armature.h"
#include "sk64/skeletool_animator.h"

struct BallLauncher {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    struct SKArmature armature;
    struct SKAnimator animator;
    short dynamicId;
    short signalIndex;
    float ballLifetime;
    float ballVelocity;
    struct Ball currentBall;
};

void ballLauncherInit(struct BallLauncher* launcher, struct BallLauncherDefinition* definition);

void ballLauncherUpdate(struct BallLauncher* launcher);

#endif