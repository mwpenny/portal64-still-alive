#include "dynamic_render_list.h"

#include "dynamic_scene.h"
#include "../util/memory.h"
#include "../physics/collision_scene.h"

extern struct DynamicScene gDynamicScene;

#define FLAG_MASK (DYNAMIC_SCENE_OBJECT_FLAGS_USED | DYNAMIC_SCENE_OBJECT_FLAGS_ACTIVE)

struct DynamicRenderDataList* dynamicRenderListNew(struct RenderState* renderState, int maxLength) {
    struct DynamicRenderDataList* result = stackMalloc(sizeof(struct DynamicRenderDataList));
    result->renderState = renderState;
    result->renderData = stackMalloc(sizeof(struct DynamicRenderData) * maxLength);
    result->maxLength = maxLength;
    result->currentLength = 0;
    result->currentRenderStateCullingMask = 0;
    result->stageCount = 0;

    if (collisionSceneIsPortalOpen()) {
        transformToMatrix(collisionSceneTransformToPortal(0), result->portalTransforms[0], SCENE_SCALE);
        transformToMatrix(collisionSceneTransformToPortal(1), result->portalTransforms[1], SCENE_SCALE);
    } else {
        guMtxIdentF(result->portalTransforms[0]);
        guMtxIdentF(result->portalTransforms[1]);
    }

    return result;
}


void dynamicRenderAddStage(struct DynamicRenderDataList* list, int exitPortalView, int parentStageIndex) {
    if (list->stageCount >= MAX_RENDER_STAGES) {
        return;
    }

    list->stages[list->stageCount].exitPortalView = exitPortalView;
    list->stages[list->stageCount].parentStageIndex = parentStageIndex;

    ++list->stageCount;
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
    next->renderStageCullingMask = list->currentRenderStateCullingMask;
}

void dynamicRenderListAddDataTouchingPortal(
    struct DynamicRenderDataList* list,
    Gfx* model,
    Mtx* transform,
    short materialIndex,
    struct Vector3* position,
    Mtx* armature,
    int rigidBodyFlags
) {
    dynamicRenderListAddData(list, model, transform, materialIndex, position, armature);

    rigidBodyFlags &= RigidBodyIsTouchingPortalA | RigidBodyWasTouchingPortalA | RigidBodyIsTouchingPortalB | RigidBodyWasTouchingPortalB;

    if (!rigidBodyFlags) {
        return;
    }

    int touchingPortalIndex = (rigidBodyFlags & (RigidBodyIsTouchingPortalA | RigidBodyWasTouchingPortalA)) ? 0 : 1;

    short parentCullingMask = 0;
    short childCullingMask = 0;

    // start at 1 because stage at index 0 cant
    // possibly have a parent
    for (int i = 1; i < list->stageCount; ++i) {
        // if cube is touching an exit portal
        // draw the cube from parent view
        if (list->stages[i].exitPortalView == touchingPortalIndex) {
            parentCullingMask |= (1 << list->stages[i].parentStageIndex);
        }

        // if cube is touching an entrance portal
        // draw the cube from exit view
        if (list->stages[list->stages[i].parentStageIndex].exitPortalView == 1 - touchingPortalIndex) {
            childCullingMask |= 1 << i;
        }
    }

    float transformAsFloat[4][4];
    guMtxL2F(transformAsFloat, transform);

    if (parentCullingMask) {
        if (list->currentLength >= list->maxLength) {
            return;
        }

        Mtx* mtx = renderStateRequestMatrices(list->renderState, 1);

        if (!mtx) {
            return;
        }

        float finalTransform[4][4];
        guMtxCatF(transformAsFloat, list->portalTransforms[touchingPortalIndex], finalTransform);

        guMtxF2L(finalTransform, mtx);

        struct DynamicRenderData* next = &list->renderData[list->currentLength];
        ++list->currentLength;

        next->model = model;
        next->transform = mtx;
        transformPoint(collisionSceneTransformToPortal(touchingPortalIndex), position, &next->position);
        next->armature = armature;
        next->materialIndex = materialIndex;
        next->renderStageCullingMask = parentCullingMask;
    }

    if (childCullingMask) {
        if (list->currentLength >= list->maxLength) {
            return;
        }

        Mtx* mtx = renderStateRequestMatrices(list->renderState, 1);

        if (!mtx) {
            return;
        }

        float finalTransform[4][4];
        guMtxCatF(transformAsFloat, list->portalTransforms[1 - touchingPortalIndex], finalTransform);

        guMtxF2L(finalTransform, mtx);

        struct DynamicRenderData* next = &list->renderData[list->currentLength];
        ++list->currentLength;

        next->model = model;
        next->transform = mtx;
        transformPoint(collisionSceneTransformToPortal(1 - touchingPortalIndex), position, &next->position);
        next->armature = armature;
        next->materialIndex = materialIndex;
        next->renderStageCullingMask = childCullingMask;
    }
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

        list->currentRenderStateCullingMask = visibleStages;

        object->renderCallback(object->data, list, renderState);
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