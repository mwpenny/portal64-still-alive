#ifndef __DYNAMIC_ASSET_LOADER_H__
#define __DYNAMIC_ASSET_LOADER_H__

#include <ultra64.h>

#include "dynamic_asset_model.h"

#include "../sk64/skeletool_armature.h"
#include "../sk64/skeletool_clip.h"

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