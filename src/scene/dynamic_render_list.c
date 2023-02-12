#include "dynamic_render_list.h"

#include "dynamic_scene.h"
#include "../util/memory.h"

extern struct DynamicScene gDynamicScene;

#define FLAG_MASK (DYNAMIC_SCENE_OBJECT_FLAGS_USED | DYNAMIC_SCENE_OBJECT_FLAGS_ACTIVE)

struct DynamicRenderDataList* dynamicRenderListNew(int maxLength) {
    struct DynamicRenderDataList* result = stackMalloc(sizeof(struct DynamicRenderDataList));
    result->renderData = stackMalloc(sizeof(struct DynamicRenderData) * maxLength);
    result->maxLength = maxLength;
    result->currentLength = 0;
    return result;
}

void dynamicRenderListFree(struct DynamicRenderDataList* list) {
    stackMallocFree(list->renderData);
    stackMallocFree(list);
}

void dynamicRenderListAddData(
    struct DynamicRenderDataList* list,
    Gfx* model,
    Mtx* transform,
    short materialIndex,
    struct Vector3* position,
    Mtx* armature
) {
    if (list->currentLength >= list->maxLength) {
        return;
    }

    struct DynamicRenderData* next = &list->renderData[list->currentLength];
    ++list->currentLength;

    next->model = model;
    next->transform = transform;
    next->position = *position;
    next->armature = armature;
    next->materialIndex = materialIndex;
}


void dynamicRenderListPopulate(struct DynamicRenderDataList* list, struct RenderProps* stages, int stageCount, struct RenderState* renderState) {
    for (int i = 0; i < MAX_DYNAMIC_SCENE_OBJECTS; ++i) {
        struct DynamicSceneObject* object = &gDynamicScene.objects[i];

        if ((object->flags & FLAG_MASK) != FLAG_MASK) {
            continue;
        }

        int visibleStages = 0;

        struct Vector3 scaledPos;
        vector3Scale(object->position, &scaledPos, SCENE_SCALE);

        for (int stageIndex = 0; stageIndex < stageCount; ++stageIndex) {
            if ((stages[stageIndex].visiblerooms & object->roomFlags) == 0) {
                continue;
            }

            if (stages[stageIndex].currentDepth == STARTING_RENDER_DEPTH && (object->flags & DYNAMIC_SCENE_OBJECT_SKIP_ROOT)) {
                continue;
            }

            if (isSphereOutsideFrustrum(&stages[stageIndex].cameraMatrixInfo.cullingInformation, &scaledPos, object->scaledRadius)) {
                continue;
            }

            visibleStages |= (1 << stageIndex);
        }

        if (!visibleStages) {
            continue;
        }

        int preCount = list->currentLength;

        object->renderCallback(object->data, list, renderState);

        for (int added = preCount; added < list->currentLength; ++added) {
            list->renderData[added].renderStageCullingMask = visibleStages;
        }
    }
}

void dynamicRenderPopulateRenderScene(
    struct DynamicRenderDataList* list, 
    int stageIndex, 
    struct RenderScene* renderScene, 
    struct Transform* cameraTransform, 
    struct FrustrumCullingInformation* cullingInfo, 
    u64 visiblerooms
) {
    int stageMask = (1 << stageIndex);
    for (int i = 0; i < list->currentLength; ++i) {
        struct DynamicRenderData* current = &list->renderData[i];

        if ((current->renderStageCullingMask & stageMask) == 0) {
            continue;
        }

        renderSceneAdd(renderScene, current->model, current->transform, current->materialIndex, &current->position, current->armature);
    }

    for (int i = 0; i < MAX_VIEW_DEPENDANT_OBJECTS; ++i) {
        struct DynamicSceneViewDependantObject* object = &gDynamicScene.viewDependantObjects[i];

        if ((object->flags & FLAG_MASK) != FLAG_MASK) {
            continue;
        }

        if ((visiblerooms & object->roomFlags) == 0) {
            continue;
        }

        struct Vector3 scaledPos;
        vector3Scale(object->position, &scaledPos, SCENE_SCALE);

        if (isSphereOutsideFrustrum(cullingInfo, &scaledPos, object->scaledRadius)) {
            continue;
        }

        object->renderCallback(object->data, renderScene, cameraTransform);
    }
}