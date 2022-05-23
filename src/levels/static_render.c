#include "static_render.h"

#include "levels.h"
#include "util/memory.h"
#include "defs.h"

u16* gRenderOrder;
u16* gRenderOrderCopy;
int* gSortKey;

void staticRenderInit() {
    gRenderOrder = malloc(sizeof(u16) * gCurrentLevel->staticContentCount);
    gRenderOrderCopy = malloc(sizeof(u16) * gCurrentLevel->staticContentCount);
    gSortKey = malloc(sizeof(int) * (gCurrentLevel->staticContentCount + MAX_DYNAMIC_OBJECTS));
}

int staticRenderSorkKeyFromMaterial(int materialIndex, float distanceScaled) {
    int distance = (int)distanceScaled;

    // sort transparent surfaces from back to front
    if (materialIndex >= levelMaterialTransparentStart()) {
        distance = 0x1000000 - distance;
    }

    return (materialIndex << 23) | (distance & 0x7FFFFF);
}

int staticRenderGenerateSortKey(int index, struct FrustrumCullingInformation* cullingInfo) {
    struct BoundingBoxs16* box = &gCurrentLevel->staticBoundingBoxes[index];
    struct Vector3 boxCenter;
    boxCenter.x = (box->minX + box->maxX) >> 1;
    boxCenter.y = (box->minY + box->maxY) >> 1;
    boxCenter.z = (box->minZ + box->maxZ) >> 1;

    int distance = (int)sqrtf(vector3DistSqrd(&boxCenter, &cullingInfo->cameraPosScaled));

    return staticRenderSorkKeyFromMaterial(gCurrentLevel->staticContent[index].materialIndex, distance);
}

void staticRenderSort(int min, int max) {
    if (min + 1 >= max) {
        return;
    }

    int middle = (min + max) >> 1;
    staticRenderSort(min, middle);
    staticRenderSort(middle, max);

    int aHead = min;
    int bHead = middle;
    int output = min;

    while (aHead < middle && bHead < max) {
        int sortDifference = gSortKey[gRenderOrder[aHead]] - gSortKey[gRenderOrder[bHead]];

        if (sortDifference <= 0) {
            gRenderOrderCopy[output] = gRenderOrder[aHead];
            ++output;
            ++aHead;
        } else {
            gRenderOrderCopy[output] = gRenderOrder[bHead];
            ++output;
            ++bHead;
        }
    }

    while (aHead < middle) {
        gRenderOrderCopy[output] = gRenderOrder[aHead];
        ++output;
        ++aHead;
    }

    while (bHead < max) {
        gRenderOrderCopy[output] = gRenderOrder[bHead];
        ++output;
        ++bHead;
    }

    for (output = min; output < max; ++output) {
        gRenderOrder[output] = gRenderOrderCopy[output];
    }
}

int staticRenderPopulateRooms(struct FrustrumCullingInformation* cullingInfo, int renderCount, u16 currentRoom, u32* visitedRooms) {
    u32 roomMask = 1 << currentRoom;

    if (*visitedRooms & roomMask) {
        return renderCount;
    }

    *visitedRooms |= roomMask;

    struct Rangeu16 staticRange = gCurrentLevel->roomStaticMapping[currentRoom];

    for (int i = staticRange.min; i < staticRange.max; ++i) {
        if (isOutsideFrustrum(cullingInfo, &gCurrentLevel->staticBoundingBoxes[i])) {
            continue;
        }

        gRenderOrder[renderCount] = i;
        gSortKey[i] = staticRenderGenerateSortKey(i, cullingInfo);
        ++renderCount;
    }

    for (int i = 0; i < gCurrentLevel->world.rooms[currentRoom].doorwayCount; ++i) {
        struct Doorway* doorway = &gCurrentLevel->world.doorways[gCurrentLevel->world.rooms[currentRoom].doorwayIndices[i]];

        if (isQuadOutsideFrustrum(cullingInfo, &doorway->quad)) {
            continue;
        }

        renderCount = staticRenderPopulateRooms(cullingInfo, renderCount, currentRoom == doorway->roomA ? doorway->roomB : doorway->roomA, visitedRooms);
    };

    return renderCount;
}

void staticRender(struct FrustrumCullingInformation* cullingInfo, u16 startingRoom, struct RenderState* renderState) {
    if (!gCurrentLevel) {
        return;
    }

    u32 visitedRooms = 0;
    int renderCount = staticRenderPopulateRooms(cullingInfo, 0, startingRoom, &visitedRooms);

    renderCount = dynamicScenePopulate(cullingInfo, renderCount, gCurrentLevel->staticContentCount, gSortKey, gRenderOrder);

    staticRenderSort(0, renderCount);

    int prevMaterial = -1;

    gSPDisplayList(renderState->dl++, levelMaterialDefault());
    
    for (int i = 0; i < renderCount; ++i) {
        int renderIndex = gRenderOrder[i];

        int materialIndex;

        if (renderIndex < gCurrentLevel->staticContentCount) {
            materialIndex = gCurrentLevel->staticContent[renderIndex].materialIndex;
        } else {
            materialIndex = dynamicSceneObjectMaterialIndex(renderIndex - gCurrentLevel->staticContentCount);
        }
    
        if (materialIndex != prevMaterial && materialIndex != -1) {
            if (prevMaterial != -1) {
                gSPDisplayList(renderState->dl++, levelMaterialRevert(prevMaterial));
            }

            gSPDisplayList(renderState->dl++, levelMaterial(materialIndex));

            prevMaterial = materialIndex;
        }

        if (renderIndex < gCurrentLevel->staticContentCount) {
            gSPDisplayList(renderState->dl++, gCurrentLevel->staticContent[renderIndex].displayList);
        } else {
            dynamicSceneRenderObject(renderIndex - gCurrentLevel->staticContentCount, renderState);
        }
    }

    if (prevMaterial != -1) {
        gSPDisplayList(renderState->dl++, levelMaterialRevert(prevMaterial));
    }
}