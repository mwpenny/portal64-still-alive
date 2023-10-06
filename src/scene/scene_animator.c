#include "scene_animator.h"

#include "../util/memory.h"
#include "../util/time.h"
#include "../math/mathf.h"
#include "../defs.h"

#include "../audio/soundplayer.h"
#include "../build/src/audio/clips.h"

struct AnimatedAudioInfo {
    short soundId;
    float pitch;
};

struct AnimatedAudioInfo gAnimatedAudioInfo[] = {
    {.soundId = SOUND_ID_NONE},
    {.soundId = SOUNDS_BEAM_PLATFORM_LOOP1, .pitch = 0.5f},
    {.soundId = SOUNDS_BEAM_PLATFORM_LOOP1, .pitch = 0.5f},
};

void sceneAnimatorInit(struct SceneAnimator* sceneAnimator, struct AnimationInfo* animationInfo, int animatorCount) {
    sceneAnimator->armatures = malloc(sizeof(struct SKArmature) * animatorCount);
    sceneAnimator->animators = malloc(sizeof(struct SKAnimator) * animatorCount);
    sceneAnimator->state = malloc(sizeof(struct SceneAnimatorState) * animatorCount);

    sceneAnimator->animationInfo = animationInfo;
    sceneAnimator->animatorCount = animatorCount;

    sceneAnimator->boneCount = 0;

    for (int i = 0; i < animatorCount; ++i) {
        skArmatureInit(&sceneAnimator->armatures[i], &animationInfo[i].armature);
        skAnimatorInit(&sceneAnimator->animators[i], animationInfo[i].armature.numberOfBones);
        sceneAnimator->state[i].playbackSpeed = 1.0f;
        sceneAnimator->state[i].soundId = SOUND_ID_NONE;
        vector3Scale(&sceneAnimator->armatures[i].pose[0].position, &sceneAnimator->state[i].lastPosition, 1.0f / SCENE_SCALE);

        sceneAnimator->boneCount += animationInfo[i].armature.numberOfBones;
    }
}

void sceneAnimatorUpdate(struct SceneAnimator* sceneAnimator) {
    for (int i = 0; i < sceneAnimator->animatorCount; ++i) {
        struct SceneAnimatorState* state = &sceneAnimator->state[i];

        skAnimatorUpdate(&sceneAnimator->animators[i], sceneAnimator->armatures[i].pose, FIXED_DELTA_TIME * state->playbackSpeed);

        struct AnimatedAudioInfo* audioInfo = &gAnimatedAudioInfo[sceneAnimator->animationInfo[i].soundType];

        if (audioInfo->soundId == SOUND_ID_NONE) {
            continue;
        }

        struct Vector3 currentPos;
        vector3Scale(&sceneAnimator->armatures[i].pose[0].position, &currentPos, 1.0f / SCENE_SCALE);
        int isMoving = currentPos.x != state->lastPosition.x || currentPos.y != state->lastPosition.y || currentPos.z != state->lastPosition.z;

        if (isMoving && state->soundId == SOUND_ID_NONE) {
            state->soundId = soundPlayerPlay(audioInfo->soundId, 1.0f, audioInfo->pitch, &currentPos, &gZeroVec);
        } else if (isMoving && state->soundId != SOUND_ID_NONE) {
            soundPlayerUpdatePosition(state->soundId, &currentPos, &gZeroVec);
        } else if (!isMoving && state->soundId != SOUND_ID_NONE) {
            soundPlayerStop(state->soundId);
            state->soundId = SOUND_ID_NONE;
        }

        state->lastPosition = currentPos;
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

    sceneAnimator->state[animatorIndex].playbackSpeed = speed;

    if (sceneAnimator->animators[animatorIndex].currentClip == clip) {
        return;
    }
    
    skAnimatorRunClip(&sceneAnimator->animators[animatorIndex], clip, speed >= 0.0f ? 0.0f : clip->nFrames / clip->fps, flags);
}

void sceneAnimatorSetSpeed(struct SceneAnimator* sceneAnimator, int animatorIndex, float speed) {
    if (animatorIndex < 0 || animatorIndex >= sceneAnimator->animatorCount) {
        return;
    }

    sceneAnimator->state[animatorIndex].playbackSpeed = speed;
}

int sceneAnimatorIsRunning(struct SceneAnimator* sceneAnimator, int animatorIndex) {
    if (animatorIndex < 0 || animatorIndex >= sceneAnimator->animatorCount) {
        return 0;
    }

    return skAnimatorIsRunning(&sceneAnimator->animators[animatorIndex]);
}

float sceneAnimatorCurrentTime(struct SceneAnimator* sceneAnimator, int animatorIndex) {
    if (animatorIndex < 0 || animatorIndex >= sceneAnimator->animatorCount) {
        return -1.0;
    }

    if (!skAnimatorIsRunning(&sceneAnimator->animators[animatorIndex])) {
        return -1.0f;
    }

    return sceneAnimator->animators[animatorIndex].currentTime;
}