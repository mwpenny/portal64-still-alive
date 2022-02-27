#ifndef _SKELATOOL_ANIMATION_CLIP_H
#define _SKELATOOL_ANIMATION_CLIP_H

#define SK_ANIMATION_EVENT_END      0xFFFF
#define SK_ANIMATION_EVENT_START    0xFFFE

enum SKBoneAttrMask {
    SKBoneAttrMaskPosition = (1 << 0),
    SKBoneAttrMaskRotation = (1 << 1),
    SKBoneAttrMaskScale = (1 << 2),

    SKBoneAttrMaskPositionConst = (1 << 4),
    SKBoneAttrMaskRotationConst = (1 << 5),
    SKBoneAttrMaskScaleConst = (1 << 6),
};

struct SKBoneKeyframe {
    unsigned char boneIndex;
    unsigned char usedAttributes;
    // each bit set in usedAttributes has 3 entries here
    short attributeData[];
};

struct SKAnimationKeyframe {
    unsigned short tick;
    unsigned short boneCount;
    struct SKBoneKeyframe bones[];
};

struct SKAnimationChunk {
    unsigned short nextChunkSize;
    unsigned short nextChunkTick;
    unsigned short keyframeCount;
    struct SKAnimationKeyframe keyframes[];
};

struct SKAnimationEvent {
    unsigned short tick;
    unsigned short id;
};

struct SKAnimationHeader {
    unsigned short firstChunkSize;
    unsigned short ticksPerSecond;
    unsigned short maxTicks;
    unsigned short numEvents;
    struct SKAnimationChunk* firstChunk;
    struct SKAnimationEvent* animationEvents;
};

#endif