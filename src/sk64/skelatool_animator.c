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

void skAnimatorCopy(u32 romAddress, void* target, u32 size) {
    if (!gAnimationPiHandle) {
        gAnimationPiHandle = osCartRomInit();
        osCreateMesgQueue(&gAnimationQueue, gAnimationQueueEntries, MAX_ANIMATION_QUEUE_ENTRIES);
    }

    // request new chunk
    OSIoMesg* ioMesg = &gAnimationIOMesg[gAnimationNextMessage];
    gAnimationNextMessage = (gAnimationNextMessage + 1) % MAX_ANIMATION_QUEUE_ENTRIES;

    ioMesg->hdr.pri = OS_MESG_PRI_NORMAL;
    ioMesg->hdr.retQueue = &gAnimationQueue;
    ioMesg->dramAddr = target;
    ioMesg->devAddr = skTranslateSegment(romAddress);
    ioMesg->size = size;

    osEPiStartDma(gAnimationPiHandle, ioMesg, OS_READ);
}

void skAnimatorV2Init(struct SKAnimatorV2* animator, int nBones) {
    animator->currentClip = NULL;
    animator->currentTime = 0.0f;
    animator->blendLerp = 0.0f;
    animator->boneState[0] = malloc(sizeof(struct SKAnimationBoneFrame) * nBones);
    animator->boneState[1] = malloc(sizeof(struct SKAnimationBoneFrame) * nBones);
    animator->boneStateFrames[0] = -1;
    animator->boneStateFrames[1] = -1;
    animator->latestBoneState = -1;
    animator->nBones = nBones;
}

void skAnimatorV2Cleanup(struct SKAnimatorV2* animator) {
    free(animator->boneState[0]);
    free(animator->boneState[1]);

    animator->boneState[0] = NULL;
    animator->boneState[1] = NULL;
}

void skAnimatorV2RequestNext(struct SKAnimatorV2* animator) {
    struct SKAnimationClip* currentClip = animator->currentClip;

    if (!currentClip) {
        return;
    }

    int nextFrame = (int)ceilf(animator->currentTime * currentClip->fps);

    if (animator->latestBoneState == -1) {
        animator->latestBoneState = 0;
    }

    if (animator->boneStateFrames[animator->latestBoneState] == nextFrame) {
        return;
    }

    animator->boneStateFrames[animator->latestBoneState] = nextFrame;

    int boneCount = MIN(animator->nBones, currentClip->nBones);

    int frameSize = currentClip->nBones * sizeof(struct SKAnimationBoneFrame);

    skAnimatorCopy((u32)currentClip->frames + frameSize * nextFrame, (void*)animator->boneState[animator->latestBoneState], sizeof(struct SKAnimationBoneFrame) * boneCount);
}

void skAnimatorV2Extractstate(struct SKAnimationBoneFrame* frames, int boneCount, struct Transform* result) {
    for (int i = 0; i < boneCount; ++i) {
        result[i].position.x = (float)frames[i].position.x;
        result[i].position.y = (float)frames[i].position.y;
        result[i].position.z = (float)frames[i].position.z;

        result[i].rotation.x = frames[i].rotation.x * (1.0f / 32767.0f);
        result[i].rotation.y = frames[i].rotation.y * (1.0f / 32767.0f);
        result[i].rotation.z = frames[i].rotation.z * (1.0f / 32767.0f);
        float wSqrd = 1.0f - (result[i].rotation.x * result[i].rotation.x + result[i].rotation.y * result[i].rotation.y + result[i].rotation.z * result[i].rotation.z);
        if (wSqrd <= 0.0f) {
            result[i].rotation.w = 0.0f;
        } else {
            result[i].rotation.w = sqrtf(wSqrd);
        }

        result[i].scale = gOneVec;
    }
}

void skAnimatorV2ReadTransform(struct SKAnimatorV2* animator, struct Transform* transforms) {
    if (animator->latestBoneState == -1) {
        return;
    }

    if (animator->blendLerp >= 1.0f) {
        skAnimatorV2Extractstate(animator->boneState[animator->latestBoneState], animator->nBones, transforms);
        return;
    }

    struct Transform fromState[animator->nBones];
    struct Transform toState[animator->nBones];

    skAnimatorV2Extractstate(animator->boneState[animator->latestBoneState], animator->nBones, toState);
    skAnimatorV2Extractstate(animator->boneState[animator->latestBoneState ^ 1], animator->nBones, fromState);

    for (int i = 0; i < animator->nBones; ++i) {
        transformLerp(&fromState[i], &toState[i], animator->blendLerp, &transforms[i]);
    }
}

void skAnimatorV2Update(struct SKAnimatorV2* animator, struct Transform* transforms, float deltaTime) {
    struct SKAnimationClip* currentClip = animator->currentClip;

    if (!currentClip) {
        return;
    }

    skAnimatorV2ReadTransform(animator, transforms);

    animator->currentTime += deltaTime;

    float currentFrameFractional = animator->currentTime * currentClip->fps;
    int currentFrame = (int)ceilf(currentFrameFractional);

    while (currentFrame >= currentClip->nFrames) {
        if (!(animator->flags & SKAnimatorV2FlagsLoop)) {
            animator->blendLerp = 1.0f;
            return;
        }
        
        animator->currentTime -= currentClip->nFrames / currentClip->fps;

        if (animator->currentTime < 0.0f) {
            animator->currentTime = 0.0f;
        }
        
        currentFrameFractional = animator->currentTime * currentClip->fps;
        currentFrame = (int)ceilf(currentFrameFractional);
    }

    int existingFrame = animator->boneStateFrames[animator->latestBoneState];
    animator->blendLerp = 1.0f - (currentFrame - currentFrameFractional);

    // no need to request the next frame
    if (existingFrame == (int)currentFrame) {
        return;
    }

    animator->latestBoneState ^= 1;

    if ((int)currentFrame > existingFrame + 1) {
        animator->blendLerp = 1.0f;
    }

    skAnimatorV2RequestNext(animator);
}

void skAnimatorV2RunClip(struct SKAnimatorV2* animator, struct SKAnimationClip* clip, float startTime, int flags) {
    animator->currentClip = clip;

    if (!clip) {
        return;
    }

    if (animator->latestBoneState != -1) {
        animator->latestBoneState ^= 1;
    }
    
    animator->boneStateFrames[0] = -1;
    animator->boneStateFrames[1] = -1;

    animator->blendLerp = 1.0f;

    animator->currentTime = startTime;
    animator->flags = flags;

    skAnimatorV2RequestNext(animator);
}

static unsigned gSegmentLocations[SK_SEGMENT_COUNT];

void skSetSegmentLocation(unsigned segmentNumber, unsigned segmentLocation) {
    gSegmentLocations[segmentNumber] = segmentLocation;
}

u32 skTranslateSegment(unsigned address) {
    unsigned segment = (address >> 24) & 0xF;
    return (address & 0xFFFFFF) + gSegmentLocations[segment];
}