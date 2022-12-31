#include "scene_animator.h"

#include "../util/memory.h"
#include "../util/time.h"

void sceneAnimatorInit(struct SceneAnimator* sceneAnimator, struct AnimationInfo* animationInfo, int animatorCount) {
    sceneAnimator->armatures = malloc(sizeof(struct SKArmature) * animatorCount);
    sceneAnimator->animators = malloc(sizeof(struct SKAnimator) * animatorCount);

    sceneAnimator->animationInfo = animationInfo;
    sceneAnimator->animatorCount = animatorCount;

    sceneAnimator->boneCount = 0;

    for (int i = 0; i < animatorCount; ++i) {
        skArmatureInit(&sceneAnimator->armatures[i], &animationInfo[i].armature);
        skAnimatorInit(&sceneAnimator->animators[i], animationInfo[i].armature.numberOfBones);

        sceneAnimator->boneCount += animationInfo[i].armature.numberOfBones;
    }
}

void sceneAnimatorUpdate(struct SceneAnimator* sceneAnimator) {
    for (int i = 0; i < sceneAnimator->animatorCount; ++i) {
        skAnimatorUpdate(&sceneAnimator->animators[i], sceneAnimator->armatures[i].pose, FIXED_DELTA_TIME);
    }
}


Mtx* sceneAnimatorBuildTransforms(struct SceneAnimator* sceneAnimator, struct RenderState* renderState) {
    Mtx* result = renderStateRequestMatrices(renderState, sceneAnimator->boneCount);

    Mtx* curr = result;

    for (int i = 0; i < sceneAnimator->animatorCount; ++i) {
        skCalculateTransforms(&sceneAnimator->armatures[i], curr);
        curr += sceneAnimator->armatures[i].numberOfBones;
    }
    
    return result;
}