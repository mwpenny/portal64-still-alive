#ifndef _SK_ANIMATION_H
#define _SK_ANIMATION_H

#include <vector>
#include <string>
#include <ostream>
#include "./CFileDefinition.h"

enum SKBoneAttrMask {
    SKBoneAttrMaskPosition = (1 << 0),
    SKBoneAttrMaskRotation = (1 << 1),
    SKBoneAttrMaskScale = (1 << 2),
};

struct SKBoneKeyframe {
    SKBoneKeyframe();
    unsigned char boneIndex;
    unsigned char usedAttributes;
    std::vector<short> attributeData;
};

struct SKAnimationKeyframe {
    SKAnimationKeyframe();
    unsigned short tick;
    std::vector<SKBoneKeyframe> bones;
};

struct SKAnimationChunk {
    SKAnimationChunk();
    unsigned short nextChunkSize;
    unsigned short nextChunkTick;
    std::vector<SKAnimationKeyframe> keyframes;
};

struct SKAnimationHeader {
    unsigned short firstChunkSize;
    unsigned short ticksPerSecond;
    unsigned short maxTicks;
    std::string animationName;
};

struct SKAnimation {
    unsigned short ticksPerSecond;
    unsigned short maxTicks;
    std::vector<SKAnimationChunk> chunks;
};

unsigned short formatAnimationChunks(const std::string& name, std::vector<SKAnimationChunk>& chunks, CFileDefinition& fileDef);

#endif