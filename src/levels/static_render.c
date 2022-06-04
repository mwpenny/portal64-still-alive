#include "static_render.h"

#include "levels.h"
#include "util/memory.h"
#include "defs.h"
#include "../graphics/render_scene.h"

void staticRenderPopulateRooms(struct FrustrumCullingInformation* cullingInfo, struct RenderScene* renderScene) {
    int currentRoom = 0;

    u64 visibleRooms = renderScene->visibleRooms;

    while (visibleRooms) {
        if (0x1 & visibleRooms) {
            struct Rangeu16 staticRange = gCurrentLevel->roomStaticMapping[currentRoom];

            for (int i = staticRange.min; i < staticRange.max; ++i) {
                struct BoundingBoxs16* box = &gCurrentLevel->staticBoundingBoxes[i];

                if (isOutsideFrustrum(cullingInfo, box)) {
                    continue;
                }

                struct Vector3 boxCenter;
                boxCenter.x = (float)(box->minX + box->maxX) * (0.5f / SCENE_SCALE);
                boxCenter.y = (float)(box->minY + box->maxY) * (0.5f / SCENE_SCALE);
                boxCenter.z = (float)(box->minZ + box->maxZ) * (0.5f / SCENE_SCALE);

                renderSceneAdd(renderScene, gCurrentLevel->staticContent[i].displayList, NULL, gCurrentLevel->staticContent[i].materialIndex, &boxCenter, NULL);
            }
        }

        visibleRooms >>= 1;
        ++currentRoom;
    }
}

void staticRenderDetermineVisibleRooms(struct FrustrumCullingInformation* cullingInfo, u16 currentRoom, u64* visitedRooms) {
    u64 roomMask = 1LL << currentRoom;

    if (*visitedRooms & roomMask) {
        return;
    }

    *visitedRooms |= roomMask;

    for (int i = 0; i < gCurrentLevel->world.rooms[currentRoom].doorwayCount; ++i) {
        struct Doorway* doorway = &gCurrentLevel->world.doorways[gCurrentLevel->world.rooms[currentRoom].doorwayIndices[i]];

        if (isQuadOutsideFrustrum(cullingInfo, &doorway->quad)) {
            continue;
        }

        staticRenderDetermineVisibleRooms(cullingInfo, currentRoom == doorway->roomA ? doorway->roomB : doorway->roomA, visitedRooms);
    };
}

int staticRenderIsRoomVisible(u64 visibleRooms, u16 roomIndex) {
    return (visibleRooms & (1LL << roomIndex)) != 0;
}

void staticRender(struct Transform* cameraTransform, struct FrustrumCullingInformation* cullingInfo, u64 visibleRooms, struct RenderState* renderState) {
    if (!gCurrentLevel) {
        return;
    }

    struct RenderScene* renderScene = renderSceneNew(cameraTransform, renderState, MAX_RENDER_COUNT, visibleRooms);

    staticRenderPopulateRooms(cullingInfo, renderScene);

    dynamicScenePopulate(cullingInfo, renderScene);

    renderSceneGenerate(renderScene, renderState);

    renderSceneFree(renderScene);
}