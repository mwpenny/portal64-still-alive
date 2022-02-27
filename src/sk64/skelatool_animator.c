
#include <assert.h>
#include "skelatool_animator.h"
#include "util/memory.h"
#include "util/time.h"
#include "math/mathf.h"

#define TICK_UNDEFINED      ~((u16)(0))

static struct SKAnimationDataPool gSKAnimationPool;

void skWaitForNextMessage();

void skRingMemoryInit(struct SKRingMemory* memory) {
    memory->freeStart = memory->memoryStart;
    memory->usedStart = memory->memoryStart;
    memory->memoryUsed = 0;
    memory->memoryWasted = 0;
}

void skRingMemoryCleanup(struct SKRingMemory* memory) {
    free(memory->memoryStart);
}

void* skRingMemoryAlloc(struct SKRingMemory* memory, int size) {
    int available = SK_POOL_SIZE - memory->memoryUsed - memory->memoryWasted;

    if (available < size) {
        return 0;
    }

    available = memory->memoryStart + SK_POOL_SIZE - memory->freeStart;

    if (available >= size) {
        char* result = memory->freeStart;
        memory->memoryUsed += size;
        memory->freeStart += size;
        return result;
    }

    int availableAtFront = memory->freeStart - memory->memoryStart;

    if (availableAtFront >= size) {
        char* result = memory->memoryStart;
        memory->memoryUsed += size;
        memory->memoryWasted = available;
        memory->freeStart = result + size;
        return result;
    }

    return 0;
}

void skRingMemoryFree(struct SKRingMemory* memory, int size) {
    memory->usedStart += size;
    memory->memoryUsed -= size;

    if (memory->usedStart + memory->memoryWasted >= memory->memoryStart + SK_POOL_SIZE) {
        memory->usedStart = memory->memoryStart;
        memory->memoryWasted = 0;
    }
}

struct SKBoneKeyframe* skApplyBoneKeyframe(struct SKAnimator* animator, struct SKBoneKeyframe* keyframe, u16 tick) {   
    short* attrInput = &keyframe->attributeData[0];

    struct SKBoneAnimationState* boneState = &animator->boneState[keyframe->boneIndex];

    if (keyframe->usedAttributes & SKBoneAttrMaskPosition) {
        boneState->prevState.positionTick = boneState->nextState.positionTick;
        boneState->prevState.position = boneState->nextState.position;
        if (boneState->nextState.flags & SKBoneAttrMaskPositionConst) {
            boneState->prevState.flags |= SKBoneAttrMaskPositionConst;
        } else {
            boneState->prevState.flags &= ~SKBoneAttrMaskPositionConst;
        }

        boneState->nextState.positionTick = tick;
        boneState->nextState.position.x = *attrInput++;
        boneState->nextState.position.y = *attrInput++;
        boneState->nextState.position.z = *attrInput++;
        if (keyframe->usedAttributes & SKBoneAttrMaskPositionConst) {
            boneState->nextState.flags |= SKBoneAttrMaskPositionConst;
        } else {
            boneState->nextState.flags &= ~SKBoneAttrMaskPositionConst;
        }
    }

    if (keyframe->usedAttributes & SKBoneAttrMaskRotation) {
        boneState->prevState.rotationTick = boneState->nextState.rotationTick;
        boneState->prevState.rotation = boneState->nextState.rotation;
        if (boneState->nextState.flags & SKBoneAttrMaskRotationConst) {
            boneState->prevState.flags |= SKBoneAttrMaskRotationConst;
        } else {
            boneState->prevState.flags &= ~SKBoneAttrMaskRotationConst;
        }

        boneState->nextState.rotationTick = tick;
        boneState->nextState.rotation.x = (*attrInput++) / 32767.0f;
        boneState->nextState.rotation.y = (*attrInput++) / 32767.0f;
        boneState->nextState.rotation.z = (*attrInput++) / 32767.0f;
        float wSqrd = 1.0f - (boneState->nextState.rotation.x * boneState->nextState.rotation.x + boneState->nextState.rotation.y * boneState->nextState.rotation.y + boneState->nextState.rotation.z * boneState->nextState.rotation.z);
        if (wSqrd <= 0.0f) {
            boneState->nextState.rotation.w = 0.0f;
        } else {
            boneState->nextState.rotation.w = sqrtf(wSqrd);
        }

        if (keyframe->usedAttributes & SKBoneAttrMaskRotationConst) {
            boneState->nextState.flags |= SKBoneAttrMaskRotationConst;
        } else {
            boneState->nextState.flags &= ~SKBoneAttrMaskRotationConst;
        }
    }

    if (keyframe->usedAttributes & SKBoneAttrMaskScale) {
        boneState->prevState.scaleTick = boneState->nextState.scaleTick;
        boneState->prevState.scale = boneState->nextState.scale;
        if (boneState->nextState.flags & SKBoneAttrMaskScaleConst) {
            boneState->prevState.flags |= SKBoneAttrMaskScaleConst;
        } else {
            boneState->prevState.flags &= ~SKBoneAttrMaskScaleConst;
        }

        boneState->nextState.scaleTick = tick;
        boneState->nextState.scale.x = *attrInput++;
        boneState->nextState.scale.y = *attrInput++;
        boneState->nextState.scale.z = *attrInput++;
        if (keyframe->usedAttributes & SKBoneAttrMaskScaleConst) {
            boneState->nextState.flags |= SKBoneAttrMaskScaleConst;
        } else {
            boneState->nextState.flags &= ~SKBoneAttrMaskScaleConst;
        }
    }

    return (struct SKBoneKeyframe*)attrInput;
}

struct SKAnimationKeyframe* skApplyKeyframe(struct SKAnimator* animator, struct SKAnimationKeyframe* keyframe) {
    struct SKBoneKeyframe* currentBone = &keyframe->bones[0];

    for (unsigned i = 0; i < keyframe->boneCount; ++i) {
        currentBone = skApplyBoneKeyframe(animator, currentBone, keyframe->tick);
    }

    return (struct SKAnimationKeyframe*)currentBone;
}


void skApplyChunk(struct SKAnimator* animator, struct SKAnimationChunk* chunk, unsigned sourceChunkStart, unsigned chunkSize) {
    struct SKAnimationKeyframe* nextFrame = &chunk->keyframes[0];

    for (unsigned i = 0; i < chunk->keyframeCount; ++i) {
        nextFrame = skApplyKeyframe(animator, nextFrame);
    }

    animator->nextChunkSource = sourceChunkStart + chunkSize;
    animator->nextSourceTick = chunk->nextChunkTick;
    animator->nextSourceChunkSize = chunk->nextChunkSize;
}

void skProcess(OSIoMesg* message) {
    int messageIndex = message - gSKAnimationPool.ioMessages;

    struct SKAnimator* animator = gSKAnimationPool.animatorsForMessages[messageIndex];

    if (animator) {
        gSKAnimationPool.animatorsForMessages[messageIndex] = 0;

        struct SKAnimationChunk* nextChunk = (struct SKAnimationChunk*)message->dramAddr;
        skApplyChunk(animator, nextChunk, message->devAddr, message->size);
        animator->flags &= ~SKAnimatorFlagsPendingRequest;
        skRingMemoryFree(&gSKAnimationPool.memoryPool, message->size);
    }
}

void skInitDataPool(OSPiHandle* handle) {
    gSKAnimationPool.handle = handle;
    osCreateMesgQueue(&gSKAnimationPool.mesgQueue, gSKAnimationPool.mesgBuffer, SK_POOL_QUEUE_SIZE);
    zeroMemory(gSKAnimationPool.segmentLocations, sizeof(gSKAnimationPool.segmentLocations));
    skResetDataPool();
}

void skResetDataPool() {
    while (skHasPendingMessages()) {
        skWaitForNextMessage();
    }

    gSKAnimationPool.nextMessage = 0;
    skRingMemoryInit(&gSKAnimationPool.memoryPool);
    zeroMemory(gSKAnimationPool.animatorsForMessages, sizeof(struct SKAnimator*) * SK_POOL_QUEUE_SIZE);
}

void skReadMessages() {
    OSMesg msg;
    while (osRecvMesg(&gSKAnimationPool.mesgQueue, &msg, OS_MESG_NOBLOCK) != -1) {
        skProcess(msg);
    }
}

int skHasPendingMessages() {
    return gSKAnimationPool.mesgQueue.validCount > 0;
}

void skSetSegmentLocation(unsigned segmentNumber, unsigned segmentLocation) {
    gSKAnimationPool.segmentLocations[segmentNumber] = segmentLocation;
}

u32 skTranslateSegment(unsigned address) {
    unsigned segment = (address >> 24) & 0xF;
    return (address & 0xFFFFFF) + gSKAnimationPool.segmentLocations[segment];
}

void skWaitForNextMessage() {
    assert(gSKAnimationPool.mesgQueue.validCount);
    OSMesg msg;
    osRecvMesg(&gSKAnimationPool.mesgQueue, &msg, OS_MESG_BLOCK);
    skProcess(msg);
}

void skWaitForPendingRequest(struct SKAnimator* animator) {
    volatile struct SKAnimator* animatorVol = animator;
    // if this animator already has an open request wait for it to finish before starting a new one
    while ((animatorVol->flags & SKAnimatorFlagsPendingRequest) != 0 && gSKAnimationPool.mesgQueue.validCount > 0) {
        skWaitForNextMessage();
    }
}

void skRequestChunk(struct SKAnimator* animator, unsigned chunkSize) {
    if (chunkSize == 0) {
        return;
    }

    if (IS_KSEG0(animator->nextChunkSource)) {
        // if the animation is in RAM apply it right away
        skApplyChunk(animator, (struct SKAnimationChunk*)animator->nextChunkSource, animator->nextChunkSource, chunkSize);
        return;
    }

    if (chunkSize > SK_POOL_SIZE || !animator->nextChunkSource) {
        // chunk can't possitbly fit exit early
        return;
    }

    unsigned short retries = 0;

    char* dest = skRingMemoryAlloc(&gSKAnimationPool.memoryPool, chunkSize);

    // wait until enough memory is avaialble
    while ((!dest || gSKAnimationPool.mesgQueue.validCount == gSKAnimationPool.mesgQueue.msgCount) && gSKAnimationPool.mesgQueue.validCount != 0) {
        // something is wrong, avoid an infinite loop
        if (retries == SK_POOL_QUEUE_SIZE) {
            animator->flags &= ~SKAnimatorFlagsActive;
            return;
        }
        skWaitForNextMessage();
        dest = skRingMemoryAlloc(&gSKAnimationPool.memoryPool, chunkSize);
        ++retries;
    }

    if (!dest) {
        animator->flags &= ~SKAnimatorFlagsActive;
        return;
    }

    skWaitForPendingRequest(animator);

    animator->flags |= SKAnimatorFlagsPendingRequest;

    // request new chunk
    OSIoMesg* ioMesg = &gSKAnimationPool.ioMessages[gSKAnimationPool.nextMessage];
    gSKAnimationPool.animatorsForMessages[gSKAnimationPool.nextMessage] = animator;
    gSKAnimationPool.nextMessage = (gSKAnimationPool.nextMessage + 1) % SK_POOL_QUEUE_SIZE;

    ioMesg->hdr.pri = OS_MESG_PRI_NORMAL;
    ioMesg->hdr.retQueue = &gSKAnimationPool.mesgQueue;
    ioMesg->dramAddr = (void*)dest;
    ioMesg->devAddr = (u32)animator->nextChunkSource;
    ioMesg->size = chunkSize;

    osEPiStartDma(gSKAnimationPool.handle, ioMesg, OS_READ);
}

void skFixedVector3ToFloat(struct SKU16Vector3* input, struct Vector3* output) {
    output->x = (float)input->x;
    output->y = (float)input->y;
    output->z = (float)input->z;
}

void skApplyBoneAnimation(struct SKBoneAnimationState* animatedBone, struct Transform* output, float tick) {
    if (animatedBone->nextState.positionTick != animatedBone->prevState.positionTick && (animatedBone->prevState.flags & SKBoneAttrMaskPositionConst) == 0) {
        float positionLerp = (tick - (float)animatedBone->prevState.positionTick) / ((float)animatedBone->nextState.positionTick - (float)animatedBone->prevState.positionTick);
        struct Vector3 srcPos;
        skFixedVector3ToFloat(&animatedBone->prevState.position, &srcPos);
        skFixedVector3ToFloat(&animatedBone->nextState.position, &output->position);
        vector3Lerp(&srcPos, &output->position, positionLerp, &output->position);
    } else {
        skFixedVector3ToFloat(&animatedBone->prevState.position, &output->position);
    }

    if (isnan(output->position.x)) output->position.x = 0.0f;
    if (isnan(output->position.y)) output->position.y = 0.0f;
    if (isnan(output->position.x)) output->position.z = 0.0f;

    if (animatedBone->nextState.rotationTick != animatedBone->prevState.rotationTick && (animatedBone->prevState.flags & SKBoneAttrMaskRotationConst) == 0) {
        float rotationLerp = (tick - (float)animatedBone->prevState.rotationTick) / ((float)animatedBone->nextState.rotationTick - (float)animatedBone->prevState.rotationTick);
        quatLerp(&animatedBone->prevState.rotation, &animatedBone->nextState.rotation, rotationLerp, &output->rotation);
    } else {
        output->rotation = animatedBone->prevState.rotation;
    }

    if (isnan(output->rotation.x) || isnan(output->rotation.y) || isnan(output->rotation.z) || isnan(output->rotation.w)) {
        quatIdent(&output->rotation);
    }

    if (animatedBone->nextState.scaleTick != animatedBone->prevState.scaleTick && (animatedBone->prevState.flags & SKBoneAttrMaskScaleConst) == 0) {
        float scaleLerp = (tick - (float)animatedBone->prevState.scaleTick) / ((float)animatedBone->nextState.scaleTick - (float)animatedBone->prevState.scaleTick);
        struct Vector3 srcScale;
        skFixedVector3ToFloat(&animatedBone->prevState.scale, &srcScale);
        skFixedVector3ToFloat(&animatedBone->nextState.scale, &output->scale);
        vector3Lerp(&srcScale, &output->scale, scaleLerp, &output->scale);
        vector3Scale(&output->scale, &output->scale, 1.0f / 256.0f);
    } else {
        skFixedVector3ToFloat(&animatedBone->prevState.scale, &output->scale);
        vector3Scale(&output->scale, &output->scale, 1.0f / 256.0f);
    }

    if (isnan(output->scale.x)) output->scale.x = 1.0f;
    if (isnan(output->scale.y)) output->scale.y = 1.0f;
    if (isnan(output->scale.x)) output->scale.z = 1.0f;
}

void skFreezeBoneAnimation(struct SKBoneAnimationState* animatedBone, float tick) {
    struct Transform transform;
    skApplyBoneAnimation(animatedBone, &transform, tick);

    animatedBone->prevState.position.x = (short)transform.position.x;
    animatedBone->prevState.position.y = (short)transform.position.y;
    animatedBone->prevState.position.z = (short)transform.position.z;

    animatedBone->prevState.rotation = transform.rotation;

    animatedBone->prevState.scale.x = (short)(transform.scale.x * 256);
    animatedBone->prevState.scale.y = (short)(transform.scale.y * 256);
    animatedBone->prevState.scale.z = (short)(transform.scale.z * 256);

    animatedBone->nextState.position.x = (short)transform.position.x;
    animatedBone->nextState.position.y = (short)transform.position.y;
    animatedBone->nextState.position.z = (short)transform.position.z;

    animatedBone->nextState.rotation = transform.rotation;

    animatedBone->nextState.scale.x = (short)(transform.scale.x * 256);
    animatedBone->nextState.scale.y = (short)(transform.scale.y * 256);
    animatedBone->nextState.scale.z = (short)(transform.scale.z * 256);
}

void skAnimatorInit(struct SKAnimator* animator, unsigned boneCount, SKAnimationEventCallback animtionCallback, void* callbackData) {
    animator->flags = 0;
    animator->boneCount = boneCount;
    animator->currentTime = 0.0f;
    animator->currTick = TICK_UNDEFINED;
    animator->nextTick = 0;
    animator->nextSourceTick = TICK_UNDEFINED;
    animator->nextSourceChunkSize = 0;
    animator->nextChunkSource = 0;
    animator->boneState = malloc(sizeof(struct SKBoneAnimationState) * boneCount); 
    zeroMemory(animator->boneState, sizeof(struct SKBoneAnimationState) * boneCount);
    animator->currentAnimation = 0;
    animator->eventCallback = animtionCallback;
    animator->eventCallbackData = callbackData;
    animator->nextEvent = 0;
}

void skAnimatorCleanup(struct SKAnimator* animator) {
    free(animator->boneState);
    animator->boneState = 0;
    animator->flags = 0;
    animator->currentAnimation = 0;
}

void skAnimatorRunClip(struct SKAnimator* animator, struct SKAnimationHeader* animation, int flags) {
    skWaitForPendingRequest(animator);

    if (animator->currTick != TICK_UNDEFINED && animator->currentAnimation) {
        float currTick = animator->currentTime * animator->currentAnimation->ticksPerSecond;

        for (unsigned i = 0; i < animator->boneCount; ++i) {
            skFreezeBoneAnimation(&animator->boneState[i], currTick);
        }
    }

    animator->flags = (unsigned short)(flags | SKAnimatorFlagsActive);
    animator->currentTime = 0.0f;
    animator->currTick = TICK_UNDEFINED;
    animator->nextTick = 0;
    animator->nextSourceTick = TICK_UNDEFINED;
    animator->nextSourceChunkSize = animation->firstChunkSize;
    if (IS_KSEG0(animation->firstChunk)) {  
        animator->nextChunkSource = (u32)animation->firstChunk;
    } else {
        animator->nextChunkSource = skTranslateSegment((u32)animation->firstChunk);
    }
    animator->currentAnimation = animation;
    animator->nextEvent = 0;

    struct SKAnimationEvent event;
    event.id = SK_ANIMATION_EVENT_START;
    event.tick = 0;

    if (animator->eventCallback) {
        animator->eventCallback(animator, animator->eventCallbackData, &event);
    }

    skRequestChunk(animator, animation->firstChunkSize);
}

void skAnimationApply(struct SKAnimator* animator, struct Transform* transforms, float tick) {
    for (unsigned i = 0; i < animator->boneCount; ++i) {
        skApplyBoneAnimation(&animator->boneState[i], &transforms[i], tick);
    }
}

void skAnimatorUpdate(struct SKAnimator* animator, struct Transform* transforms, float timeScale) {
    if (!(animator->flags & SKAnimatorFlagsActive) || !animator->currentAnimation) {
        return;
    }

    while (animator->flags & SKAnimatorFlagsPendingRequest) {
        skWaitForPendingRequest(animator);
    }
    
    animator->currTick = animator->nextTick;
    float currTick = animator->currentTime * animator->currentAnimation->ticksPerSecond;
    animator->currentTime += gTimeDelta * timeScale;
    animator->nextTick = (u16)(animator->currentTime * animator->currentAnimation->ticksPerSecond);

    if (animator->currTick <= animator->currentAnimation->maxTicks && transforms) {
        skAnimationApply(animator, transforms, currTick);
    } else if (animator->currTick >= animator->currentAnimation->maxTicks) {
        animator->flags &= ~SKAnimatorFlagsActive;
        animator->currentAnimation = 0;
        return;
    }

    animator->flags |= SKAnimatorFlagsUpdating;

    while (animator->eventCallback && animator->nextEvent < animator->currentAnimation->numEvents && 
        animator->currTick >= animator->currentAnimation->animationEvents[animator->nextEvent].tick &&
        animator->nextSourceTick >= animator->currentAnimation->animationEvents[animator->nextEvent].tick) {
        animator->eventCallback(animator, animator->eventCallbackData, &animator->currentAnimation->animationEvents[animator->nextEvent]);

        // if the event callback started a new animation exit early
        if (!(animator->flags & SKAnimatorFlagsUpdating)) {
            return;
        }
        
        ++animator->nextEvent;
    }

    if (animator->nextTick >= animator->currentAnimation->maxTicks) {
        struct SKAnimationEvent event;
        event.id = SK_ANIMATION_EVENT_END;
        event.tick = animator->currentAnimation->maxTicks;

        if (animator->eventCallback) {
            animator->eventCallback(animator, animator->eventCallbackData, &event);
        }

        // if the event callback started a new animation exit early
        if (!(animator->flags & SKAnimatorFlagsUpdating)) {
            return;
        }

        if (animator->flags & SKAnimatorFlagsLoop) {
            skAnimatorRunClip(animator, animator->currentAnimation, animator->flags);
            return;
        }
    }

    animator->flags &= ~SKAnimatorFlagsUpdating;
    
    // queue up next keyframes if they are needed
    if (animator->nextTick >= animator->nextSourceTick) {
        animator->nextSourceTick = TICK_UNDEFINED;
        skRequestChunk(animator, animator->nextSourceChunkSize);
    }
}

int skAnimatorIsRunning(struct SKAnimator* animator) {
    return (animator->flags & SKAnimatorFlagsActive) != 0 && animator->currentAnimation;
}

float skAnimationLength(struct SKAnimationHeader* clip) {
    return (float)clip->maxTicks / (float)clip->ticksPerSecond;
}