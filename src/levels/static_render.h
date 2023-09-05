#ifndef __STATIC_RENDER_H__
#define __STATIC_RENDER_H__

#include "level_definition.h"
#include "graphics/renderstate.h"
#include "scene/camera.h"
#include "../scene/dynamic_render_list.h"

void staticRenderDetermineVisibleRooms(struct FrustrumCullingInformation* cullingInfo, u16 currentRoom, u64* visitedRooms);
int staticRenderIsRoomVisible(u64 visibleRooms, u16 roomIndex);
void staticRender(struct Transform* cameraTransform, struct FrustrumCullingInformation* cullingInfo, u64 visibleRooms, struct DynamicRenderDataList* dynamicList, int stageIndex, Mtx* staticTransforms, struct RenderState* renderState);

void staticRenderCheckSignalMaterials();

#endif