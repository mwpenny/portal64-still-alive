#ifndef __DYNAMIC_ASSET_LOADER_H__
#define __DYNAMIC_ASSET_LOADER_H__

#include <ultra64.h>

struct DynamicAssetModel {
    void* addressStart;
    void* addressEnd;
    void* segmentStart;
    Gfx* model;
};

void dynamicAssetsReset();

void dynamicAssetModelPreload(int index);
Gfx* dynamicAssetModel(int index);

void* dynamicAssetFixPointer(int index, void* ptr);

#endif