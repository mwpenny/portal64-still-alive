#include "render_scene.h"

#include "../util/memory.h"
#include "defs.h"
#include "../levels/levels.h"
#include "sk64/skelatool_defs.h"

struct RenderScene* renderSceneNew(struct Transform* cameraTransform, struct RenderState *renderState, int capacity, u64 visibleRooms) {
    struct RenderScene* result = stackMalloc(sizeof(struct RenderScene));

    struct Vector3 cameraForward;
    quatMultVector(&cameraTransform->rotation, &gForward, &cameraForward);
    vector3Negate(&cameraForward, &cameraForward);
    planeInitWithNormalAndPoint(&result->forwardPlane, &cameraForward, &cameraTransform->position);
    
    result->currentRenderPart = 0;
    result->maxRenderParts = capacity;

    result->renderParts = stackMalloc(sizeof(struct RenderPart) * capacity);
    result->sortKeys = stackMalloc(sizeof(int) * capacity);
    result->materials = stackMalloc(sizeof(short) * capacity);

    result->renderOrder = NULL;
    result->renderOrderCopy = NULL;

    result->visibleRooms = visibleRooms;

    result->renderState = renderState;

    return result;
}

void renderSceneFree(struct RenderScene* renderScene) {
    stackMallocFree(renderScene);
}

int renderSceneSortKey(int materialIndex, float distance) {
    int distanceScaled = (int)(distance * SCENE_SCALE);

    // sort transparent surfaces from back to front
    if (materialIndex >= levelMaterialTransparentStart()) {
        return (0xFF << 23) | (0x1000000 - distanceScaled);
    }

    return (materialIndex << 23) | (distanceScaled & 0x7FFFFF);
}

void renderSceneAdd(struct RenderScene* renderScene, Gfx* geometry, Mtx* matrix, int materialIndex, struct Vector3* at, Mtx* armature) {
    if (renderScene->currentRenderPart == renderScene->maxRenderParts) {
        return;
    }

    struct RenderPart* part = &renderScene->renderParts[renderScene->currentRenderPart];
    part->geometry = geometry;
    part->matrix = matrix;
    part->armature = armature;
    renderScene->materials[renderScene->currentRenderPart] = materialIndex;
    renderScene->sortKeys[renderScene->currentRenderPart] = renderSceneSortKey(materialIndex, planePointDistance(&renderScene->forwardPlane, at));

    ++renderScene->currentRenderPart;
}

void renderSceneSort(struct RenderScene* renderScene, int min, int max) {
    if (min + 1 >= max) {
        return;
    }

    int middle = (min + max) >> 1;
    renderSceneSort(renderScene, min, middle);
    renderSceneSort(renderScene, middle, max);

    int aHead = min;
    int bHead = middle;
    int output = min;

    while (aHead < middle && bHead < max) {
        int sortDifference = renderScene->sortKeys[renderScene->renderOrder[aHead]] - renderScene->sortKeys[renderScene->renderOrder[bHead]];

        if (sortDifference <= 0) {
            renderScene->renderOrderCopy[output] = renderScene->renderOrder[aHead];
            ++output;
            ++aHead;
        } else {
            renderScene->renderOrderCopy[output] = renderScene->renderOrder[bHead];
            ++output;
            ++bHead;
        }
    }

    while (aHead < middle) {
        renderScene->renderOrderCopy[output] = renderScene->renderOrder[aHead];
        ++output;
        ++aHead;
    }

    while (bHead < max) {
        renderScene->renderOrderCopy[output] = renderScene->renderOrder[bHead];
        ++output;
        ++bHead;
    }

    for (output = min; output < max; ++output) {
        renderScene->renderOrder[output] = renderScene->renderOrderCopy[output];
    }
}

void renderSceneGenerate(struct RenderScene* renderScene, struct RenderState* renderState) {
    renderScene->renderOrder = stackMalloc(sizeof(short) * renderScene->currentRenderPart);
    renderScene->renderOrderCopy = stackMalloc(sizeof(short) * renderScene->currentRenderPart);

    for (int i = 0; i < renderScene->currentRenderPart; ++i) {
        renderScene->renderOrder[i] = i;
    }

    renderSceneSort(renderScene, 0, renderScene->currentRenderPart);

    int prevMaterial = -1;

    gSPDisplayList(renderState->dl++, levelMaterialDefault());
    
    for (int i = 0; i < renderScene->currentRenderPart; ++i) {
        int renderIndex = renderScene->renderOrder[i];

        int materialIndex = renderScene->materials[renderIndex];
    
        if (materialIndex != prevMaterial && materialIndex != -1) {
            if (prevMaterial != -1) {
                gSPDisplayList(renderState->dl++, levelMaterialRevert(prevMaterial));
            }

            gSPDisplayList(renderState->dl++, levelMaterial(materialIndex));

            prevMaterial = materialIndex;
        }

        struct RenderPart* renderPart = &renderScene->renderParts[renderIndex];

        if (renderPart->matrix) {
            gSPMatrix(renderState->dl++, renderPart->matrix, G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL);
        }

        if (renderPart->armature) {
            gSPSegment(renderState->dl++, MATRIX_TRANSFORM_SEGMENT, renderPart->armature);
        }

        gSPDisplayList(renderState->dl++, renderPart->geometry);

        if (renderPart->matrix) {
            gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
        }
    }

    if (prevMaterial != -1) {
        gSPDisplayList(renderState->dl++, levelMaterialRevert(prevMaterial));
    }
}
