#ifndef __SCENE_ANIMATOR_H__
#define __SCENE_ANIMATOR_H__

#include "../levels/level_definition.h"

#include "../sk64/skelatool_armature.h"
#include "../sk64/skelatool_animator.h"

#include "../graphics/renderstate.h"

enum SceneAnimatorStateFlags {
    SceneAnimatorStateWasMoving = (1 << 0),
};

struct SceneAnimatorState {
    float playbackSpeed;
    ALSndId soundId;
    short flags;
    struct Vector3 lastPosition;
};

struct SceneAnimator {
    struct SKArmature* armatures;
    struct SKAnimator* animators;
    struct AnimationInfo* animationInfo;
    struct SceneAnimatorState* state;
    struct Transform* transforms;
    short animatorCount;
    short boneCount;
};

void sceneAnimatorInit(struct SceneAnimator* sceneAnimator, struct AnimationInfo* animationInfo, int animatorCount);

void sceneAnimatorUpdate(struct SceneAnimator* sceneAnimator);

void sceneAnimatorTransformForIndex(struct SceneAnimator* sceneAnimator, int index, struct Transform* result);

Mtx* sceneAnimatorBuildTransforms(struct SceneAnimator* sceneAnimator, struct RenderState* renderState);

void sceneAnimatorPlay(struct SceneAnimator* sceneAnimator, int animatorIndex, int animationIndex, float speed, int flags);
void sceneAnimatorSetSpeed(struct SceneAnimator* sceneAnimator, int animatorIndex, float speed);

int sceneAnimatorIsRunning(struct SceneAnimator* sceneAnimator, int animatorIndex);

float sceneAnimatorCurrentTime(struct SceneAnimator* sceneAnimator, int animatorIndex);

#endif