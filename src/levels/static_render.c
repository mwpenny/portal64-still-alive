#include "static_render.h"

#include "levels.h"
#include "util/memory.h"

u16* gRenderOrder;
u16* gRenderOrderCopy;
int* gSortKey;

void staticRenderInit() {
    gRenderOrder = malloc(sizeof(u16) * gCurrentLevel->staticContentCount);
    gRenderOrderCopy = malloc(sizeof(u16) * gCurrentLevel->staticContentCount);
    gSortKey = malloc(sizeof(int) * gCurrentLevel->staticContentCount);
}

int staticRenderGenerateSortKey(int index) {
    // TODO determine distance
    return (int)gCurrentLevel->staticContent[index].materialIndex << 23;
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

int isOutsideFrustrum(struct FrustrumCullingInformation* frustrum, struct BoundingBoxs16* boundingBox) {
    for (int i = 0; i < CLIPPING_PLANE_COUNT; ++i) {
        struct Vector3 closestPoint;

        closestPoint.x = frustrum->clippingPlanes[i].normal.x < 0.0f ? boundingBox->minX : boundingBox->maxX;
        closestPoint.y = frustrum->clippingPlanes[i].normal.y < 0.0f ? boundingBox->minY : boundingBox->maxY;
        closestPoint.z = frustrum->clippingPlanes[i].normal.z < 0.0f ? boundingBox->minZ : boundingBox->maxZ;

        if (planePointDistance(&frustrum->clippingPlanes[i], &closestPoint) < 0.0f) {
            return 1;
        }
    }


    return 0;
}

void staticRender(struct FrustrumCullingInformation* cullingInfo, struct RenderState* renderState) {
    if (!gCurrentLevel) {
        return;
    }

    int renderCount = 0;

    for (int i = 0; i < gCurrentLevel->staticContentCount; ++i) {
        if (isOutsideFrustrum(cullingInfo, &gCurrentLevel->staticBoundingBoxes[i])) {
            continue;
        }

        // TODO filter
        gRenderOrder[renderCount] = i;
        gSortKey[renderCount] = staticRenderGenerateSortKey(i);
        ++renderCount;
    }

    staticRenderSort(0, renderCount);

    int prevMaterial = -1;
    
    for (int i = 0; i < renderCount; ++i) {
        struct StaticContentElement* element = &gCurrentLevel->staticContent[gRenderOrder[i]];
        
        if (element->materialIndex != prevMaterial) {
            if (prevMaterial != -1) {
                gSPDisplayList(renderState->dl++, levelMaterialRevert(prevMaterial));
            }

            gSPDisplayList(renderState->dl++, levelMaterial(element->materialIndex));

            prevMaterial = element->materialIndex;
        }

        gSPDisplayList(renderState->dl++, element->displayList);
    }

    if (prevMaterial != -1) {
        gSPDisplayList(renderState->dl++, levelMaterialRevert(prevMaterial));
    }
}