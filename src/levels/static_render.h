#ifndef __STATIC_RENDER_H__
#define __STATIC_RENDER_H__

#include "graphics/renderstate.h"
#include "level_definition.h"
#include "scene/camera.h"
#include "scene/dynamic_render_list.h"
#include "scene/render_plan.h"

void staticRenderDetermineVisibleRooms(
    struct FrustumCullingInformation* rootCullingInfo,
    struct FrustumCullingInformation* cullingInfo,
    u16 currentRoom,
    u64* visitedRooms,
    u64 nonVisibleRooms
);
int staticRenderIsRoomVisible(u64 visibleRooms, u16 roomIndex);
void staticRender(
    struct RenderProps* renderStage,
    struct DynamicRenderDataList* dynamicList,
    int stageIndex,
    Mtx* staticMatrices,
    struct Transform* staticTransforms,
    struct RenderState* renderState
);

void staticRenderCheckSignalMaterials();

#endif