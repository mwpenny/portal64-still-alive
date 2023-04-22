#include "scene_animator.h"

#include "../util/memory.h"
#include "../util/time.h"
#include "../math/mathf.h"
#include "../defs.h"

void sceneAnimatorInit(struct SceneAnimator* sceneAnimator, struct AnimationInfo* animationInfo, int animatorCount) {
    sceneAnimator->armatures = malloc(sizeof(struct SKArmature) * animatorCount);
    sceneAnimator->animators = malloc(sizeof(struct SKAnimator) * animatorCount);
    sceneAnimator->playbackSpeeds = malloc(sizeof(float) * animatorCount);

    sceneAnimator->animationInfo = animationInfo;
    sceneAnimator->animatorCount = animatorCount;

    sceneAnimator->boneCount = 0;

    for (int i = 0; i < animatorCount; ++i) {
        skArmatureInit(&sceneAnimator->armatures[i], &animationInfo[i].armature);
        skAnimatorInit(&sceneAnimator->animators[i], animationInfo[i].armature.numberOfBones);
        sceneAnimator->playbackSpeeds[i] = 1.0f;

        sceneAnimator->boneCount += animationInfo[i].armature.numberOfBones;
    }
}

void sceneAnimatorUpdate(struct SceneAnimator* sceneAnimator) {
    for (int i = 0; i < sceneAnimator->animatorCount; ++i) {
        skAnimatorUpdate(&sceneAnimator->animators[i], sceneAnimator->armatures[i].pose, FIXED_DELTA_TIME * sceneAnimator->playbackSpeeds[i]);
    }
}

void sceneAnimatorTransformForIndex(struct SceneAnimator* sceneAnimator, int index, struct Transform* result) {
    for (int i = 0; i < sceneAnimator->animatorCount; ++i) {
        if (index < sceneAnimator->armatures[i].numberOfBones) {
            *result = sceneAnimator->armatures[i].pose[index];
            vector3Scale(&result->position, &result->position, 1.0f / SCENE_SCALE);
            return;
        }

        index -= sceneAnimator->armatures[i].numberOfBones;
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

void sceneAnimatorPlay(struct SceneAnimator* sceneAnimator, int animatorIndex, int animationIndex, float speed, int flags) {
    if (animatorIndex < 0 || animatorIndex >= sceneAnimator->animatorCount) {
        return;
    }

    struct AnimationInfo* info = &sceneAnimator->animationInfo[animatorIndex];

    if (animationIndex < 0 || animationIndex >= info->clipCount) {
        return;
    }

    struct SKAnimationClip* clip = &info->clips[animationIndex];

    sceneAnimator->playbackSpeeds[animatorIndex] = speed;

    if (sceneAnimator->animators[animatorIndex].currentClip == clip) {
        return;
    }
    
    skAnimatorRunClip(&sceneAnimator->animators[animatorIndex], clip, speed >= 0.0f ? 0.0f : clip->nFrames / clip->fps, flags);
}

void sceneAnimatorSetSpeed(struct SceneAnimator* sceneAnimator, int animatorIndex, float speed) {
    if (animatorIndex < 0 || animatorIndex >= sceneAnimator->animatorCount) {
        return;
    }

    sceneAnimator->playbackSpeeds[animatorIndex] = speed;
}

int sceneAnimatorIsRunning(struct SceneAnimator* sceneAnimator, int animatorIndex) {
    if (animatorIndex < 0 || animatorIndex >= sceneAnimator->animatorCount) {
        return 0;
    }

    return skAnimatorIsRunning(&sceneAnimator->animators[animatorIndex]);
}