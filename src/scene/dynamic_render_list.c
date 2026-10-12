#include "dynamic_render_list.h"

#include "dynamic_scene.h"
#include "physics/collision_scene.h"
#include "savefile/savefile.h"
#include "util/memory.h"

extern struct DynamicScene gDynamicScene;

struct DynamicRenderDataList* dynamicRenderListNew(struct RenderState* renderState, struct RenderProps* renderStages, int renderStageCount, int maxLength) {
    struct DynamicRenderDataList* result = stackMalloc(sizeof(struct DynamicRenderDataList));
    result->renderState = renderState;
    result->renderData = stackMalloc(sizeof(struct DynamicRenderData) * maxLength);
    result->maxLength = maxLength;
    result->currentLength = 0;
    result->currentRenderStateCullingMask = 0;
    result->renderStageCount = renderStageCount;
    result->renderStages = renderStages;
    result->visibleRooms = 0;

    for (int stageIndex = 0; stageIndex < result->renderStageCount; ++stageIndex) {
        struct RenderProps* stage = &result->renderStages[stageIndex];
        result->visibleRooms |= stage->visiblerooms;
    }

    if (collisionSceneIsPortalOpen()) {
        transformToMatrix(collisionSceneTransformToOtherPortal(0), result->portalTransforms[0], SCENE_SCALE);
        transformToMatrix(collisionSceneTransformToOtherPortal(1), result->portalTransforms[1], SCENE_SCALE);
    } else {
        guMtxIdentF(result->portalTransforms[0]);
        guMtxIdentF(result->portalTransforms[1]);
    }

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

    int portalFlags = rigidBodyFlags & (RigidBodyIsTouchingPortal0 | RigidBodyWasTouchingPortal0 | RigidBodyIsTouchingPortal1 | RigidBodyWasTouchingPortal1);
    if (!portalFlags) {
        return;
    }

    // Find render stages where clone should be shown
    short cullingMask = 0;
    int touchingPortalIndex = (portalFlags & (RigidBodyIsTouchingPortal0 | RigidBodyWasTouchingPortal0)) ? 0 : 1;

    // Start at 1 because stage 0 has no parents
    for (int i = 1; i < list->renderStageCount; ++i) {
        struct RenderProps* stage = &list->renderStages[i];

        if (stage->exitPortalIndex == touchingPortalIndex) {
            // If object is touching an exit portal, draw from parent view
            // (makes it not disappear moving out of portal before crossing)
            cullingMask |= (1 << stage->parentStageIndex);
        } else if (!(rigidBodyFlags & RigidBodyIsPlayer) || stage->parentStageIndex != 0) {
            // If object is touching an entrance portal, draw from child view
            // (makes it not disappear moving into portal before crossing)
            cullingMask |= 1 << i;
        }
    }

    if (!cullingMask || list->currentLength >= list->maxLength) {
        return;
    }

    // Render clones
    Mtx* mtx = renderStateRequestMatrices(list->renderState, 1);
    if (!mtx) {
        return;
    }

    float transformAsFloat[4][4];
    float finalTransform[4][4];
    guMtxL2F(transformAsFloat, transform);
    guMtxCatF(transformAsFloat, list->portalTransforms[touchingPortalIndex], finalTransform);
    guMtxF2L(finalTransform, mtx);

    struct DynamicRenderData* next = &list->renderData[list->currentLength++];
    next->model = model;
    next->transform = mtx;
    transformPoint(collisionSceneTransformToOtherPortal(touchingPortalIndex), position, &next->position);
    next->armature = armature;
    next->materialIndex = materialIndex;
    next->renderStageCullingMask = cullingMask;
}

static int isDynamicObjectCulled(struct DynamicSceneObject* object, struct Vector3* scaledPos, struct RenderProps* renderStage) {
    // Coarse culling
    if (isSphereOutsideFrustum(&renderStage->cameraMatrixInfo.cullingInformation, scaledPos, object->scaledRadius)) {
        return 1;
    }

    // The coarse bounding sphere can clip through the back of portals.
    // Check more precisely when near the exit portal, to avoid showing
    // from behind when viewing from the parent stage.
    if (object->preciseCullingCallback &&
        renderStage->currentDepth < gSaveData.gameplay.portalRenderDepth
    ) {
        struct Vector3* portalPos = &gCollisionScene.portalTransforms[renderStage->exitPortalIndex]->position;
        float distThreshold = (object->scaledRadius * (1.0f / SCENE_SCALE)) + PORTAL_COVER_HEIGHT_RADIUS;

        if (vector3DistSqrd(portalPos, object->position) <= (distThreshold * distThreshold)) {
            return object->preciseCullingCallback(object->data, &renderStage->cameraMatrixInfo.cullingInformation);
        }
    }

    return 0;
}

void dynamicRenderListPopulate(struct DynamicRenderDataList* list) {
    for (int i = 0; i < MAX_DYNAMIC_SCENE_OBJECTS; ++i) {
        struct DynamicSceneObject* object = &gDynamicScene.objects[i];

        if (!(object->flags & DYNAMIC_SCENE_OBJECT_FLAGS_USED)) {
            continue;
        }

        int visibleStages = 0;

        struct Vector3 scaledPos;
        vector3Scale(object->position, &scaledPos, SCENE_SCALE);

        for (int stageIndex = 0; stageIndex < list->renderStageCount; ++stageIndex) {
            struct RenderProps* stage = &list->renderStages[stageIndex];

            if ((stage->visiblerooms & object->roomFlags) == 0) {
                continue;
            }

            if (stage->currentDepth == gSaveData.gameplay.portalRenderDepth && (object->flags & DYNAMIC_SCENE_OBJECT_SKIP_ROOT)) {
                continue;
            }

            if (isDynamicObjectCulled(object, &scaledPos, stage)) {
                continue;
            }

            visibleStages |= (1 << stageIndex);
        }

        if (!visibleStages) {
            continue;
        }

        list->currentRenderStateCullingMask = visibleStages;

        object->renderCallback(object->data, list, list->renderState);
    }
}

void dynamicRenderPopulateRenderScene(
    struct DynamicRenderDataList* list,
    int stageIndex,
    struct RenderScene* renderScene
) {
    int stageMask = (1 << stageIndex);
    for (int i = 0; i < list->currentLength; ++i) {
        struct DynamicRenderData* current = &list->renderData[i];

        if ((current->renderStageCullingMask & stageMask) == 0) {
            continue;
        }

        renderSceneAdd(renderScene, current->model, current->transform, current->materialIndex, &current->position, current->armature);
    }

    struct RenderProps* stage = &list->renderStages[stageIndex];

    for (int i = 0; i < MAX_VIEW_DEPENDENT_OBJECTS; ++i) {
        struct DynamicSceneObject* object = &gDynamicScene.viewDependentObjects[i];

        if (!(object->flags & DYNAMIC_SCENE_OBJECT_FLAGS_USED)) {
            continue;
        }

        if ((stage->visiblerooms & object->roomFlags) == 0) {
            continue;
        }

        struct Vector3 scaledPos;
        vector3Scale(object->position, &scaledPos, SCENE_SCALE);

        if (isDynamicObjectCulled(object, &scaledPos, stage)) {
            continue;
        }

        object->viewRenderCallback(object->data, renderScene, &stage->camera.transform);
    }
}
