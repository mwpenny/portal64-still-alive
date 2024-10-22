#ifndef __DYNAMIC_ASSET_MODEL_H__
#define __DYNAMIC_ASSET_MODEL_H__

#include <ultra64.h>

#include "../sk64/skeletool_armature.h"
#include "../sk64/skeletool_clip.h"

struct DynamicAssetModel {
    void* addressStart;
    void* addressEnd;
    void* segmentStart;
    Gfx* model;
    char* name;
};

struct DynamicAnimatedAssetModel {
    void* addressStart;
    void* addressEnd;
    void* segmentStart;
    struct SKArmatureDefinition* armature;
    struct SKAnimationClip** clips;
    short clipCount;
    char* name;
};

#endif
