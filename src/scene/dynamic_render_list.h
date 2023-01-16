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

struct DynamicRenderDataList {
    struct DynamicRenderData* renderData;
    short maxLength;
    short currentLength;
};

struct DynamicRenderDataList* dynamicRenderListNew(int maxLength);
void dynamicRenderListFree(struct DynamicRenderDataList* list);

void dynamicRenderListAddData(
    struct DynamicRenderDataList* list,
    Gfx* model,
    Mtx* transform,
    short materialIndex,
    struct Vector3* position,
    Mtx* armature
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