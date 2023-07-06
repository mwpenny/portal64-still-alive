#include "skelatool_animator.h"

#include "../util/memory.h"
#include "../math/mathf.h"
#include "skelatool_animator.h"

#define MAX_ANIMATION_QUEUE_ENTRIES 20

OSPiHandle* gAnimationPiHandle;
OSMesgQueue gAnimationQueue;
OSMesg gAnimationQueueEntries[MAX_ANIMATION_QUEUE_ENTRIES];
OSIoMesg gAnimationIOMesg[MAX_ANIMATION_QUEUE_ENTRIES];
int gAnimationNextMessage;
int gPendingAnimationRequests;

void skAnimatorCopy(u32 romAddress, void* target, u32 size) {
    if (!gAnimationPiHandle) {
        gAnimationPiHandle = osCartRomInit();
        osCreateMesgQueue(&gAnimationQueue, gAnimationQueueEntries, MAX_ANIMATION_QUEUE_ENTRIES);
    }

    if (gPendingAnimationRequests == MAX_ANIMATION_QUEUE_ENTRIES) {
        OSMesg msg;
        osRecvMesg(&gAnimationQueue, &msg, OS_MESG_BLOCK);
        --gPendingAnimationRequests;
    }

    // request new chunk
    OSIoMesg* ioMesg = &gAnimationIOMesg[gAnimationNextMessage];
    gAnimationNextMessage = (gAnimationNextMessage + 1) % MAX_ANIMATION_QUEUE_ENTRIES;

    osInvalDCache((void*)target, size);

    ioMesg->hdr.pri = OS_MESG_PRI_NORMAL;
    ioMesg->hdr.retQueue = &gAnimationQueue;
    ioMesg->dramAddr = target;
    ioMesg->devAddr = skTranslateSegment(romAddress);
    ioMesg->size = size;

    osEPiStartDma(gAnimationPiHandle, ioMesg, OS_READ);
    ++gPendingAnimationRequests;
}

void skAnimatorSync() {
    while (gPendingAnimationRequests) {
        OSMesg msg;
        osRecvMesg(&gAnimationQueue, &msg, OS_MESG_BLOCK);
        --gPendingAnimationRequests;
    }
}

void skAnimatorInit(struct SKAnimator* animator, int nBones) {
    animator->currentClip = NULL;
    animator->currentTime = 0.0f;
    animator->blendLerp = 0.0f;
    animator->boneState[0] = malloc(sizeof(struct SKAnimationBoneFrame) * nBones);
    animator->boneState[1] = malloc(sizeof(struct SKAnimationBoneFrame) * nBones);
    animator->boneStateFrames[0] = -1;
    animator->boneStateFrames[1] = -1;
    animator->nextFrameStateIndex = -1;
    animator->nBones = nBones;
}

void skAnimatorCleanup(struct SKAnimator* animator) {
    free(animator->boneState[0]);
    free(animator->boneState[1]);

    animator->boneState[0] = NULL;
    animator->boneState[1] = NULL;
}

void skAnimatorRequestFrame(struct SKAnimator* animator, int nextFrame) {
    struct SKAnimationClip* currentClip = animator->currentClip;

    if (!currentClip) {
        return;
    }

    if (nextFrame < 0 || nextFrame >= currentClip->nFrames) {
        return;
    }

    if (animator->nextFrameStateIndex == -1) {
        animator->nextFrameStateIndex = 0;
    }

    if (animator->boneStateFrames[animator->nextFrameStateIndex] == nextFrame) {
        return;
    }

    animator->boneStateFrames[animator->nextFrameStateIndex] = nextFrame;

    int boneCount = MIN(animator->nBones, currentClip->nBones);

    int frameSize = currentClip->nBones * sizeof(struct SKAnimationBoneFrame);

    skAnimatorCopy((u32)currentClip->frames + frameSize * nextFrame, (void*)animator->boneState[animator->nextFrameStateIndex], sizeof(struct SKAnimationBoneFrame) * boneCount);
}

void skAnimatorExtractBone(struct SKAnimationBoneFrame* bone, struct Transform* result) {
    result->position.x = (float)bone->position.x;
    result->position.y = (float)bone->position.y;
    result->position.z = (float)bone->position.z;

    result->rotation.x = bone->rotation.x * (1.0f / 32767.0f);
    result->rotation.y = bone->rotation.y * (1.0f / 32767.0f);
    result->rotation.z = bone->rotation.z * (1.0f / 32767.0f);
    float wSqrd = 1.0f - (result->rotation.x * result->rotation.x + result->rotation.y * result->rotation.y + result->rotation.z * result->rotation.z);
    if (wSqrd <= 0.0f) {
        result->rotation.w = 0.0f;
    } else {
        result->rotation.w = sqrtf(wSqrd);
    }
}

void skAnimatorInitZeroTransform(struct SKAnimator* animator, struct Transform* transforms) {
    if (animator->nextFrameStateIndex == -1) {
        return;
    }

    for (int i = 0; i < animator->nBones; ++i) {
        transforms[i].position = gZeroVec;
        transforms[i].rotation = gQuaternionZero;
        transforms[i].scale = gOneVec;
    }
}

void skAnimatorNormalize(struct SKAnimator* animator, struct Transform* transforms) {
    for (int i = 0; i < animator->nBones; ++i) {
        quatNormalize(&transforms[i].rotation, &transforms[i].rotation);
    }
}

void skAnimatorBlendTransform(struct SKAnimationBoneFrame* frame, struct Transform* transforms, int nBones, float weight) {
    for (int i = 0; i < nBones; ++i) {
        struct Transform boneTransform;
        skAnimatorExtractBone(&frame[i], &boneTransform);
        
        vector3AddScaled(&transforms[i].position, &boneTransform.position, weight, &transforms[i].position);

        if (quatDot(&transforms[i].rotation, &boneTransform.rotation) < 0) {
            transforms[i].rotation.x -= boneTransform.rotation.x * weight;
            transforms[i].rotation.y -= boneTransform.rotation.y * weight;
            transforms[i].rotation.z -= boneTransform.rotation.z * weight;
            transforms[i].rotation.w -= boneTransform.rotation.w * weight;
        } else {
            transforms[i].rotation.x += boneTransform.rotation.x * weight;
            transforms[i].rotation.y += boneTransform.rotation.y * weight;
            transforms[i].rotation.z += boneTransform.rotation.z * weight;
            transforms[i].rotation.w += boneTransform.rotation.w * weight;
        }
        
    }
}

void skAnimatorReadTransformWithWeight(struct SKAnimator* animator, struct Transform* transforms, float weight) {
    if (animator->blendLerp >= 1.0f) {
        skAnimatorBlendTransform(animator->boneState[animator->nextFrameStateIndex], transforms, animator->nBones, weight);
        return;
    }

    skAnimatorBlendTransform(animator->boneState[animator->nextFrameStateIndex], transforms, animator->nBones, animator->blendLerp * weight);
    skAnimatorBlendTransform(animator->boneState[animator->nextFrameStateIndex ^ 1], transforms, animator->nBones, (1.0f - animator->blendLerp) * weight);
}

void skAnimatorReadTransform(struct SKAnimator* animator, struct Transform* transforms) {
    if (animator->nextFrameStateIndex == -1) {
        return;
    }

    skAnimatorInitZeroTransform(animator, transforms);
    skAnimatorReadTransformWithWeight(animator, transforms, 1.0f);
    skAnimatorNormalize(animator, transforms);
}

int skAnimatorBoneStateIndexOfFrame(struct SKAnimator* animator, int frame) {
    if (animator->boneStateFrames[0] == frame) {
        return 0;
    }

    if (animator->boneStateFrames[1] == frame) {
        return 1;
    }

    return -1;
}

int skAnimatorClampFrame(struct SKAnimator* animator, int frame) {
    if (frame < animator->currentClip->nFrames) {
        return frame;
    }

    if (animator->flags & SKAnimatorFlagsLoop) {
        return frame - animator->currentClip->nFrames;
    }

    return animator->currentClip->nFrames - 1;
}

void skAnimatorStep(struct SKAnimator* animator, float deltaTime) {
    struct SKAnimationClip* currentClip = animator->currentClip;

    if (!currentClip) {
        return;
    }

    animator->currentTime += deltaTime;

    float duration = currentClip->nFrames / currentClip->fps;

    if ((animator->currentTime >= duration && deltaTime > 0.0f) || (animator->currentTime < 0.0f && deltaTime < 0.0f)) {
        if (animator->flags & SKAnimatorFlagsLoop) {
            animator->currentTime = mathfMod(animator->currentTime, duration);
        } else {
            animator->currentTime = minf(duration, maxf(0.0f, animator->currentTime));
            animator->flags |= SKAnimatorFlagsDone;
        }
    }

    float currentFrameFractional = animator->currentTime * currentClip->fps;
    int prevFrame = (int)floorf(currentFrameFractional);
    int nextFrame = (int)ceilf(currentFrameFractional);
    float lerpValue = currentFrameFractional - prevFrame;

    prevFrame = skAnimatorClampFrame(animator, prevFrame);
    nextFrame = skAnimatorClampFrame(animator, nextFrame);

    if (nextFrame == prevFrame) {
        lerpValue = 1.0f;
    }

    int existingPrevFrame = skAnimatorBoneStateIndexOfFrame(animator, prevFrame);
    int existingNextFrame = skAnimatorBoneStateIndexOfFrame(animator, nextFrame);

    if (existingPrevFrame == -1 && existingNextFrame == -1) {
        // both frames need to be requested
        animator->blendLerp = lerpValue;
        if (prevFrame != nextFrame) {
            animator->nextFrameStateIndex = 0;
            skAnimatorRequestFrame(animator, prevFrame);
        }
        animator->nextFrameStateIndex = 1;
        skAnimatorRequestFrame(animator, nextFrame);
        return;
    }

    if (existingNextFrame == -1) {
        // only the next frame needs to be requested
        animator->blendLerp = lerpValue;
        animator->nextFrameStateIndex = existingPrevFrame ^ 1;
        skAnimatorRequestFrame(animator, nextFrame);
        return;
    }

    if (existingPrevFrame == -1) {
        // only the previous frame needs to be requested
        animator->blendLerp = 1.0f - lerpValue;
        animator->nextFrameStateIndex = existingNextFrame ^ 1;
        skAnimatorRequestFrame(animator, prevFrame);
        return;
    }

    if (existingNextFrame == existingPrevFrame) {
        // only one frame is needed and is already present
        animator->blendLerp = 1.0f;
        animator->nextFrameStateIndex = existingNextFrame;
        return;
    }

    if (existingNextFrame == 1) {
        animator->blendLerp = lerpValue;
    } else {
        animator->blendLerp = 1.0f - lerpValue;
    }
}

void skAnimatorUpdate(struct SKAnimator* animator, struct Transform* transforms, float deltaTime) {
    struct SKAnimationClip* currentClip = animator->currentClip;

    if (!currentClip) {
        return;
    }

    skAnimatorReadTransform(animator, transforms);

    if (animator->flags & SKAnimatorFlagsDone) {
        animator->currentClip = NULL;
        return;
    }

    skAnimatorStep(animator, deltaTime);
}

void skAnimatorRunClip(struct SKAnimator* animator, struct SKAnimationClip* clip, float startTime, int flags) {
    animator->currentClip = clip;

    if (!clip) {
        return;
    }

    if (animator->nextFrameStateIndex != -1) {
        animator->nextFrameStateIndex ^= 1;
    }
    
    animator->boneStateFrames[0] = -1;
    animator->boneStateFrames[1] = -1;

    animator->blendLerp = 1.0f;

    animator->currentTime = startTime;
    animator->flags = flags;

    skAnimatorStep(animator, 0.0f);
}

int skAnimatorIsRunning(struct SKAnimator* animator) {
    return animator->currentClip != NULL;
}

static unsigned gSegmentLocations[SK_SEGMENT_COUNT];

void skSetSegmentLocation(unsigned segmentNumber, unsigned segmentLocation) {
    gSegmentLocations[segmentNumber] = segmentLocation;
}

u32 skTranslateSegment(unsigned address) {
    unsigned segment = (address >> 24) & 0xF;
    return (address & 0xFFFFFF) + gSegmentLocations[segment];
}

void skBlenderInit(struct SKAnimatorBlender* blender, int nBones) {
    skAnimatorInit(&blender->from, nBones);
    skAnimatorInit(&blender->to, nBones);
    blender->blendLerp = 0.0f;
}

void skBlenderApply(struct SKAnimatorBlender* blender, struct Transform* transforms) {
    float lerp = blender->blendLerp;

    if (blender->from.nextFrameStateIndex == -1) {
        if (blender->to.nextFrameStateIndex == -1) {
            return;
        }

        lerp = 1.0f;
    }

    if (blender->to.nextFrameStateIndex == -1) {
        lerp = 0.0f;
    }

    skAnimatorInitZeroTransform(&blender->from, transforms);

    if (lerp == 1.0f) {
        skAnimatorReadTransformWithWeight(&blender->to, transforms, 1.0f);
    } else if (lerp == 0.0f) {
        skAnimatorReadTransformWithWeight(&blender->from, transforms, 1.0f);
    } else {
        skAnimatorReadTransformWithWeight(&blender->to, transforms, lerp);
        skAnimatorReadTransformWithWeight(&blender->from, transforms, 1.0f - lerp);
    }
}

void skBlenderCleanup(struct SKAnimatorBlender* blender) {
    skAnimatorCleanup(&blender->from);
    skAnimatorCleanup(&blender->to);
}

void skBlenderUpdate(struct SKAnimatorBlender* blender, struct Transform* transforms, float deltaTime) {
    skBlenderApply(blender, transforms);
    skAnimatorStep(&blender->from, deltaTime);
    skAnimatorStep(&blender->to, deltaTime);
}