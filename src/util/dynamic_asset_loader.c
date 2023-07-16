#include "dynamic_asset_loader.h"
#include "memory.h"
#include "rom.h"

#include "../build/assets/models/dynamic_model_list.h"
#include "../build/assets/models/dynamic_animated_model_list.h"

extern struct DynamicAssetModel gDynamicModels[];
extern struct DynamicAnimatedAssetModel gDynamicAnimatedModels[];

Gfx* gLoadedModels[DYNAMIC_MODEL_COUNT];
u32 gModelPointerOffset[DYNAMIC_MODEL_COUNT];

struct SKArmatureWithAnimations gLoadedAnimatedModels[DYNAMIC_ANIMATED_MODEL_COUNT];

Gfx gBlankGfx[] = {
    gsSPEndDisplayList(),
};

struct SKArmatureDefinition gBlankArmature = {
    gBlankGfx,
    NULL,
    NULL,
    0,
    0,
};

struct SKArmatureWithAnimations gBlankArmatureWithAnimations = {
    &gBlankArmature,
    NULL,
    0,
};

struct SKAnimationClip gBlankClip = {
    0,
    0,
    NULL,
    0,
};

void dynamicAssetsReset() {
    zeroMemory(gLoadedModels, sizeof(gLoadedModels));
    zeroMemory(gLoadedAnimatedModels, sizeof(gLoadedAnimatedModels));
}

#define ADJUST_POINTER_POS(ptr, offset) (void*)((ptr) ? (char*)(ptr) + (offset) : 0)

Gfx* dynamicAssetFixModelPointers(Gfx* gfx, u32 pointerOffset, u32 segmentStart, u32 segmentEnd) {
    // only adjust pointers that land within the segment
    if ((u32)gfx < segmentStart || (u32)gfx >= segmentEnd) {
        return gfx;
    }

    gfx = ADJUST_POINTER_POS(gfx, pointerOffset);

    Gfx* result = gfx;

    // prevent an infinite loop if G_ENDL isn't found
    Gfx* maxGfx = ADJUST_POINTER_POS(segmentEnd, pointerOffset);
    while (gfx < maxGfx) {
        int commandType = _SHIFTR(gfx->words.w0, 24, 8);

        switch (commandType) {
            case G_ENDDL:
                return result;
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

Gfx* dynamicAssetLoadModel(struct DynamicAssetModel* model, u32* pointerOffset) {
    u32 length = (u32)model->addressEnd - (u32)model->addressStart;
    void* assetMemoryChunk = malloc(length);
    romCopy(model->addressStart, assetMemoryChunk, length);
    *pointerOffset = (u32)assetMemoryChunk - (u32)model->segmentStart;
    Gfx* result = dynamicAssetFixModelPointers(model->model, *pointerOffset, (u32)model->segmentStart, (u32)model->segmentStart + length);

    if (!result) {
        free(assetMemoryChunk);
        *pointerOffset = 0;
    }

    return result;
}

void dynamicAssetLoadAnimatedModel(struct DynamicAnimatedAssetModel* model, struct SKArmatureWithAnimations* result) {
    u32 length = (u32)model->addressEnd - (u32)model->addressStart;
    void* assetMemoryChunk = malloc(length);
    romCopy(model->addressStart, assetMemoryChunk, length);
    u32 pointerOffset = (u32)assetMemoryChunk - (u32)model->segmentStart;

    result->armature = ADJUST_POINTER_POS(model->armature, pointerOffset);

    result->armature->displayList = dynamicAssetFixModelPointers(result->armature->displayList, pointerOffset, (u32)model->segmentStart, (u32)model->segmentStart + length);
    result->armature->pose = ADJUST_POINTER_POS(result->armature->pose, pointerOffset);
    result->armature->boneParentIndex = ADJUST_POINTER_POS(result->armature->boneParentIndex, pointerOffset);

    result->clips = ADJUST_POINTER_POS(model->clips, pointerOffset);

    for (int i = 0; i < model->clipCount; i++) {
        result->clips[i] = ADJUST_POINTER_POS(result->clips[i], pointerOffset);
    }

    result->clipCount = model->clipCount;
}

void dynamicAssetModelPreload(int index) {
    if (index < 0 || index >= DYNAMIC_MODEL_COUNT || gLoadedModels[index] ) {
        return;
    }

    gLoadedModels[index] = dynamicAssetLoadModel(&gDynamicModels[index], &gModelPointerOffset[index]);
}

Gfx* dynamicAssetModel(int index) {
    if (index < 0 || index >= DYNAMIC_MODEL_COUNT || !gLoadedModels[index]) {
        return gBlankGfx;
    }

    return gLoadedModels[index];
}

void* dynamicAssetFixPointer(int index, void* ptr) {
    if (index < 0 || index >= DYNAMIC_MODEL_COUNT || !gLoadedModels[index]) {
        return NULL;
    }

    return (void*)ADJUST_POINTER_POS(ptr, gModelPointerOffset[index]);
}

struct SKArmatureWithAnimations* dynamicAssetAnimatedModel(int index) {
    if (index < 0 || index >= DYNAMIC_ANIMATED_MODEL_COUNT) {
        return &gBlankArmatureWithAnimations;
    }

    if (!gLoadedAnimatedModels[index].armature) {
        dynamicAssetLoadAnimatedModel(&gDynamicAnimatedModels[index], &gLoadedAnimatedModels[index]);
    }

    if (!gLoadedAnimatedModels[index].armature) {
        return &gBlankArmatureWithAnimations;
    }

    return &gLoadedAnimatedModels[index];
}

struct SKAnimationClip* dynamicAssetClip(int index, int clipIndex) {
    struct SKArmatureWithAnimations* armature = dynamicAssetAnimatedModel(index);

    if (clipIndex < 0 || clipIndex >= armature->clipCount) {
        return &gBlankClip;
    }

    return armature->clips[clipIndex];
}