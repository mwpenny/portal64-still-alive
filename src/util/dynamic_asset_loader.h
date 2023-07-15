#ifndef __DYNAMIC_ASSET_LOADER_H__
#define __DYNAMIC_ASSET_LOADER_H__

#include <ultra64.h>

struct DynamicAssetModel {
    void* addressStart;
    void* addressEnd;
    void* segmentStart;
    Gfx* model;
};

Gfx* dynamicAssetLoadModel(struct DynamicAssetModel* model);

void dynamicAssetsReset();

void dynamicAssetPreload(int index);
Gfx* dynamicAssetModel(int index);

#endif