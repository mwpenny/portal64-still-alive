#include "dynamic_asset_loader.h"
#include "memory.h"
#include "rom.h"

#include "../build/assets/models/dynamic_model_list.h"

extern struct DynamicAssetModel gDynamicModels[];
Gfx* gLoadedModels[DYNAMIC_MODEL_COUNT];

Gfx gBlankGfx[] = {
    gsSPEndDisplayList(),
};

void dynamicAssetsReset() {
    zeroMemory(gLoadedModels, sizeof(gLoadedModels));
}

#define ADJUST_POINTER_POS(ptr, offset) (void*)((ptr) ? (char*)(ptr) + (offset) : 0)

Gfx* dynamicAssetFixModelPointers(Gfx* gfx, u32 pointerOffset, u32 segmentStart, u32 segmentEnd) {
    // only adjust pointers that land within the segment
    if ((u32)gfx < segmentStart || (u32)gfx >= segmentEnd) {
        return gfx;
    }

    gfx = ADJUST_POINTER_POS(gfx, pointerOffset);
    while (1) {
        int commandType = _SHIFTR(gfx->words.w0, 24, 8);

        switch (commandType) {
            case G_ENDDL:
                return gfx;
            case G_DL:
                if (gfx->dma.par == G_DL_NOPUSH) {
                    break;
                }
                gfx->dma.addr = (u32)dynamicAssetFixModelPointers((Gfx*)gfx->dma.addr, pointerOffset, segmentStart, segmentEnd);
                break;
            case G_VTX:
                gfx->dma.addr = (u32)ADJUST_POINTER_POS(gfx->dma.addr, pointerOffset);
                break;
        };

        ++gfx;
    }

    return NULL;
}

Gfx* dynamicAssetLoadModel(struct DynamicAssetModel* model) {
    u32 length = (u32)model->addressEnd - (u32)model->addressStart;
    void* assetMemoryChunk = malloc(length);
    romCopy(model->addressStart, assetMemoryChunk, length);
    u32 pointerOffset = (u32)assetMemoryChunk - (u32)model->segmentStart;
    return dynamicAssetFixModelPointers(model->model, pointerOffset, (u32)model->segmentStart, (u32)model->segmentStart + length);
}

void dynamicAssetPreload(int index) {
    if (index < 0 || index >= DYNAMIC_MODEL_COUNT || gLoadedModels[index] ) {
        return;
    }

    gLoadedModels[index] = dynamicAssetLoadModel(&gDynamicModels[index]);
}

Gfx* dynamicAssetModel(int index) {
    if (index < 0 || index >= DYNAMIC_MODEL_COUNT || !gLoadedModels[index]) {
        return gBlankGfx;
    }

    return gLoadedModels[index];
}