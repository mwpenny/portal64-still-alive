#ifndef __SCENE_BALL_CATCHER_H__
#define __SCENE_BALL_CATCHER_H__


#include "../sk64/skelatool_armature.h"
#include "../sk64/skelatool_animator.h"
#include "../physics/collision_object.h"

#include "../levels/level_definition.h"

struct BallCatcher {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    struct SKArmature armature;
    struct SKAnimator animator;
    short dynamicId;
    short signalIndex;
};

void ballCatcherInit(struct BallCatcher* catcher, struct BallCatcherDefinition* definition);
void ballCatcherUpdate(struct BallCatcher* catcher);

#endif