#ifndef __SCENE_DYNAMIC_RENDER_LIST_H__
#define __SCENE_DYNAMIC_RENDER_LIST_H__

#include <ultra64.h>
#include "../math/vector3.h"
#include "render_plan.h"
#include "../graphics/render_scene.h"

struct DynamicRenderData {
    Gfx* model;
    Mtx* transform;
    struct Vector3 position;
    Mtx* armature;
    short materialIndex;
    short renderStageCullingMask;
};

#define MAX_RENDER_STAGES   6

struct DynamicRenderStageData {
    short exitPortalView;
    short parentStageIndex;
};

struct DynamicRenderDataList {
    struct RenderState* renderState;
    struct DynamicRenderData* renderData;
    short maxLength;
    short currentLength;
    short currentRenderStateCullingMask;
    short stageCount;
    struct DynamicRenderStageData stages[MAX_RENDER_STAGES];
    float portalTransforms[2][4][4];
};

struct DynamicRenderDataList* dynamicRenderListNew(struct RenderState* renderState, int maxLength);
void dynamicRenderListFree(struct DynamicRenderDataList* list);

void dynamicRenderAddStage(struct DynamicRenderDataList* list, int exitPortalView, int parentStageIndex);

void dynamicRenderListAddData(
    struct DynamicRenderDataList* list,
    Gfx* model,
    Mtx* transform,
    short materialIndex,
    struct Vector3* position,
    Mtx* armature
);

void dynamicRenderListAddDataTouchingPortal(
    struct DynamicRenderDataList* list,
    Gfx* model,
    Mtx* transform,
    short materialIndex,
    struct Vector3* position,
    Mtx* armature,
    int rigidBodyFlags
);

void dynamicRenderListPopulate(struct DynamicRenderDataList* list, struct RenderProps* stages, int stageCount, struct RenderState* renderState);
void dynamicRenderPopulateRenderScene(
    struct DynamicRenderDataList* list, 
    int stageIndex, 
    struct RenderScene* renderScene, 
    struct Transform* cameraTransform, 
    struct FrustrumCullingInformation* cullingInfo, 
    u64 visibleRooms
);

#endif