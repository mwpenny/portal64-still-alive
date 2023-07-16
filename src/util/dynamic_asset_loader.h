#ifndef __DYNAMIC_ASSET_LOADER_H__
#define __DYNAMIC_ASSET_LOADER_H__

#include <ultra64.h>

#include "../sk64/skelatool_armature.h"
#include "../sk64/skelatool_clip.h"

struct DynamicAssetModel {
    void* addressStart;
    void* addressEnd;
    void* segmentStart;
    Gfx* model;
};

struct DynamicAnimatedAssetModel {
    void* addressStart;
    void* addressEnd;
    void* segmentStart;
    struct SKArmatureDefinition* armature;
    struct SKAnimationClip** clips;
    short clipCount;
};

struct SKArmatureWithAnimations {
    struct SKArmatureDefinition* armature;
    struct SKAnimationClip** clips;
    short clipCount;
};

void dynamicAssetsReset();

void dynamicAssetModelPreload(int index);
Gfx* dynamicAssetModel(int index);

void* dynamicAssetFixPointer(int index, void* ptr);

struct SKArmatureWithAnimations* dynamicAssetAnimatedModel(int index);
struct SKAnimationClip* dynamicAssetClip(int index, int clipIndex);

#endif