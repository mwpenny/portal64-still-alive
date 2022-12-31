#ifndef __SCENE_ANIMATOR_H__
#define __SCENE_ANIMATOR_H__

#include "../levels/level_definition.h"

#include "../sk64/skelatool_armature.h"
#include "../sk64/skelatool_animator.h"

#include "../graphics/renderstate.h"

struct SceneAnimator {
    struct SKArmature* armatures;
    struct SKAnimator* animators;
    struct AnimationInfo* animationInfo;
    short animatorCount;
    short boneCount;
};

void sceneAnimatorInit(struct SceneAnimator* sceneAnimator, struct AnimationInfo* animationInfo, int animatorCount);

void sceneAnimatorUpdate(struct SceneAnimator* sceneAnimator);

Mtx* sceneAnimatorBuildTransforms(struct SceneAnimator* sceneAnimator, struct RenderState* renderState);

#endif