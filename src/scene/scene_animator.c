#include "scene_animator.h"

#include "audio/soundplayer.h"
#include "defs.h"
#include "math/mathf.h"
#include "system/time.h"
#include "util/memory.h"

#include "codegen/assets/audio/clips.h"

struct AnimatedAudioInfo {
    short startSoundId;
    short loopSoundId;
    short endSoundId;
    float volume;
    float pitch;
};

struct AnimatedAudioInfo gAnimatedAudioInfo[] = {
    {.startSoundId = SOUND_ID_NONE, .loopSoundId = SOUND_ID_NONE, .endSoundId = SOUND_ID_NONE},
    {.startSoundId = SOUND_ID_NONE, .loopSoundId = SOUNDS_BEAM_PLATFORM_LOOP1, .endSoundId = SOUND_ID_NONE, .volume = 1.0f, .pitch = 1.0f},
    {.startSoundId = SOUNDS_DOORMOVE1, .loopSoundId = SOUND_ID_NONE, .endSoundId = SOUND_ID_NONE, .volume = 1.0f, .pitch = 0.8f},
    {.startSoundId = SOUNDS_TANK_TURRET_START1, .loopSoundId = SOUNDS_TANK_TURRET_LOOP1, .endSoundId = SOUNDS_ELEVATOR_STOP1, .volume = 2.0f, .pitch = 1.0f},
    {.startSoundId = SOUNDS_APC_START_LOOP3, .loopSoundId = SOUNDS_DOOR_METAL_MEDIUM_OPEN1, .endSoundId = SOUNDS_APC_SHUTDOWN, .volume = 1.0f, .pitch = 1.0f},
    {.startSoundId = SOUNDS_DOOR_METAL_THIN_CLOSE2, .loopSoundId = SOUND_ID_NONE, .endSoundId = SOUND_ID_NONE, .volume = 1.0f, .pitch = 1.0f},
};

void sceneAnimatorInit(struct SceneAnimator* sceneAnimator, struct AnimationInfo* animationInfo, int animatorCount) {
    sceneAnimator->armatures = malloc(sizeof(struct SKArmature) * animatorCount);
    sceneAnimator->animators = malloc(sizeof(struct SKAnimator) * animatorCount);
    sceneAnimator->state = malloc(sizeof(struct SceneAnimatorState) * animatorCount);

    sceneAnimator->animationInfo = animationInfo;
    sceneAnimator->animatorCount = animatorCount;

    sceneAnimator->boneCount = 0;

    for (int i = 0; i < animatorCount; ++i) {
        sceneAnimator->boneCount += animationInfo[i].armature.numberOfBones;
    }

    sceneAnimator->transforms = malloc(sizeof(struct Transform) * sceneAnimator->boneCount);

    struct Transform* pose = sceneAnimator->transforms;

    for (int i = 0; i < animatorCount; ++i) {
        skArmatureInitWithPose(&sceneAnimator->armatures[i], &animationInfo[i].armature, pose);
        skAnimatorInit(&sceneAnimator->animators[i], animationInfo[i].armature.numberOfBones);
        sceneAnimator->state[i].playbackSpeed = 1.0f;
        sceneAnimator->state[i].soundId = SOUND_ID_NONE;
        sceneAnimator->state[i].flags = 0;

        struct Vector3* lastPos = &sceneAnimator->state[i].lastPosition;
        skArmatureGetCenter(&sceneAnimator->armatures[i], lastPos);
        vector3Scale(lastPos, lastPos, 1.0f / SCENE_SCALE);

        pose += animationInfo[i].armature.numberOfBones;
    }
}

void sceneAnimatorUpdate(struct SceneAnimator* sceneAnimator) {
    for (int i = 0; i < sceneAnimator->animatorCount; ++i) {
        struct SceneAnimatorState* state = &sceneAnimator->state[i];
        struct SKAnimator* animator = &sceneAnimator->animators[i];

        if (!skAnimatorIsRunning(animator)) {
            continue;
        }

        skAnimatorUpdate(animator, sceneAnimator->armatures[i].pose, FIXED_DELTA_TIME * state->playbackSpeed);

        struct AnimatedAudioInfo* audioInfo = &gAnimatedAudioInfo[sceneAnimator->animationInfo[i].soundType];

        struct Vector3 currentPos;
        skArmatureGetCenter(&sceneAnimator->armatures[i], &currentPos);
        vector3Scale(&currentPos, &currentPos, 1.0f / SCENE_SCALE);

        int isMoving = currentPos.x != state->lastPosition.x || currentPos.y != state->lastPosition.y || currentPos.z != state->lastPosition.z;
        int wasMoving = (state->flags & SceneAnimatorStateWasMoving) != 0;

        if (audioInfo->loopSoundId != SOUND_ID_NONE) {
            if (isMoving && state->soundId == SOUND_ID_NONE) {
                state->soundId = soundPlayerPlay(audioInfo->loopSoundId, audioInfo->volume, audioInfo->pitch, &currentPos, &gZeroVec, SoundTypeAll);
            } else if (isMoving && state->soundId != SOUND_ID_NONE) {
                soundPlayerUpdatePosition(state->soundId, &currentPos, &gZeroVec);
            } else if (!isMoving && state->soundId != SOUND_ID_NONE) {
                soundPlayerStop(state->soundId);
                state->soundId = SOUND_ID_NONE;
            }
        }

        if (isMoving && !wasMoving && audioInfo->startSoundId != SOUND_ID_NONE) {
            soundPlayerPlay(audioInfo->startSoundId, 1.0f, audioInfo->pitch, &currentPos, &gZeroVec, SoundTypeAll);
        }

        if (wasMoving && !isMoving && audioInfo->endSoundId != SOUND_ID_NONE) {
            soundPlayerPlay(audioInfo->endSoundId, 1.0f, audioInfo->pitch, &currentPos, &gZeroVec, SoundTypeAll);
        }

        state->lastPosition = currentPos;
        state->flags &= ~SceneAnimatorStateWasMoving;
        if (isMoving) {
            state->flags |= SceneAnimatorStateWasMoving;
        }
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