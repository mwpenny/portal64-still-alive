#include "static_render.h"

#include "levels.h"
#include "util/memory.h"
#include "defs.h"
#include "../graphics/render_scene.h"
#include "../math/mathf.h"
#include "../math/rotated_box.h"
#include "../scene/signals.h"

#include "../build/assets/materials/static.h"

void staticRenderTraverseIndex(
    struct StaticContentBox* box, 
    struct StaticContentBox* boxEnd, 
    struct StaticContentElement* staticContent, 
    struct FrustrumCullingInformation* cullingInfo, 
    struct RenderScene* renderScene
) {
    struct StaticContentBox* fullyVisibleEnd = box;    

    while (box < boxEnd) {
        if (box >= fullyVisibleEnd) {
            enum FrustrumResult cullResult = isOutsideFrustrum(cullingInfo, &box->box);

            if (cullResult == FrustrumResultOutisde) {
                // skip all children
                box = box + box->siblingOffset;
                continue;
            } else if (cullResult == FrustrumResultInside) {
                fullyVisibleEnd = box + box->siblingOffset;
            }
        }

        for (int i = box->staticRange.min; i < box->staticRange.max; ++i) {
            renderSceneAdd(
                renderScene, 
                staticContent[i].displayList, 
                NULL, 
                staticContent[i].materialIndex, 
                &staticContent[i].center, 
                NULL
            );
        }

        box += 1;
    }
}

void staticRenderPopulateRooms(struct FrustrumCullingInformation* cullingInfo, Mtx* staticMatrices, struct Transform* staticTransforms, struct RenderScene* renderScene) {
    int currentRoom = 0;

    u64 visibleRooms = renderScene->visibleRooms;

    while (visibleRooms) {
        if (0x1 & visibleRooms) {
            struct StaticIndex* roomIndex = &gCurrentLevel->roomBvhList[currentRoom];
    
            staticRenderTraverseIndex(roomIndex->boxIndex, roomIndex->boxIndex + roomIndex->boxCount, gCurrentLevel->staticContent, cullingInfo, renderScene);

            struct BoundingBoxs16* animatedBox = roomIndex->animatedBoxes;

            for (int i = roomIndex->animatedRange.min; i < roomIndex->animatedRange.max; ++i, ++animatedBox) {
                struct StaticContentElement* staticElement = &gCurrentLevel->staticContent[i];

                struct RotatedBox rotatedBox;

                struct Transform* transform = &staticTransforms[staticElement->transformIndex];

                rotatedBoxTransform(transform, animatedBox, &rotatedBox);

                if (isRotatedBoxOutsideFrustrum(cullingInfo, &rotatedBox)) {
                    continue;
                }

                struct Vector3 center;
                vector3AddScaled(&staticElement->center, &transform->position, 1.0f / SCENE_SCALE, &center);

                renderSceneAdd(
                    renderScene, 
                    staticElement->displayList, 
                    &staticMatrices[staticElement->transformIndex], 
                    staticElement->materialIndex, 
                    &center, 
                    NULL
                );
            }
        }

        visibleRooms >>= 1;
        ++currentRoom;
    }
}

#define FORCE_RENDER_DOORWAY_DISTANCE   0.1f

void staticRenderDetermineVisibleRooms(struct FrustrumCullingInformation* cullingInfo, u16 currentRoom, u64* visitedRooms) {
    if (currentRoom == RIGID_BODY_NO_ROOM) {
        return;
    }

    u64 roomMask = 1LL << currentRoom;

    if (*visitedRooms & roomMask) {
        return;
    }

    *visitedRooms |= roomMask;

    for (int i = 0; i < gCurrentLevel->world.rooms[currentRoom].doorwayCount; ++i) {
        struct Doorway* doorway = &gCurrentLevel->world.doorways[gCurrentLevel->world.rooms[currentRoom].doorwayIndices[i]];

        if ((doorway->flags & DoorwayFlagsOpen) == 0) {
            continue;
        }

        float doorwayDistance = planePointDistance(&doorway->quad.plane, &cullingInfo->cameraPos);

        if (
            // if the player is close enough to the doorway it should still render it, even if facing the wrong way
            (fabsf(doorwayDistance) > FORCE_RENDER_DOORWAY_DISTANCE || collisionQuadDetermineEdges(&cullingInfo->cameraPos, &doorway->quad)) && 
            isQuadOutsideFrustrum(cullingInfo, &doorway->quad)) {
            continue;
        }

        staticRenderDetermineVisibleRooms(cullingInfo, currentRoom == doorway->roomA ? doorway->roomB : doorway->roomA, visitedRooms);
    };
}

int staticRenderIsRoomVisible(u64 visibleRooms, u16 roomIndex) {
    return (visibleRooms & (1LL << roomIndex)) != 0;
}

void staticRender(struct Transform* cameraTransform, struct FrustrumCullingInformation* cullingInfo, u64 visibleRooms, struct DynamicRenderDataList* dynamicList, int stageIndex, Mtx* staticMatrices, struct Transform* staticTransforms, struct RenderState* renderState) {
    if (!gCurrentLevel) {
        return;
    }

    struct RenderScene* renderScene = renderSceneNew(cameraTransform, renderState, MAX_RENDER_COUNT, visibleRooms);

    staticRenderPopulateRooms(cullingInfo, staticMatrices, staticTransforms, renderScene);
    dynamicRenderPopulateRenderScene(dynamicList, stageIndex, renderScene, cameraTransform, cullingInfo, visibleRooms);
    renderSceneGenerate(renderScene, renderState);

    renderSceneFree(renderScene);
}

u8 gSignalMaterialMapping[] = {
    INDICATOR_LIGHTS_INDEX, INDICATOR_LIGHTS_ON_INDEX,
    SIGNAGE_DOORSTATE_INDEX, SIGNAGE_DOORSTATE_ON_INDEX,
};

void staticRenderCheckSignalMaterials() {
    for (int signal = 0; signal < gCurrentLevel->signalToStaticCount; ++signal) {
        int currentSignal = signalsRead(signal);

        if (currentSignal != signalsReadPrevious(signal)) {
            struct Rangeu16* range = &gCurrentLevel->signalToStaticRanges[signal];

            int toIndex = currentSignal ? 1 : 0;
            int fromIndex = currentSignal ? 0 : 1;

            for (int index = range->min; index < range->max; ++index) {
                struct StaticContentElement* element = &gCurrentLevel->staticContent[gCurrentLevel->signalToStaticIndices[index]];

                for (int materialIndex = 0; materialIndex < sizeof(gSignalMaterialMapping) / sizeof(*gSignalMaterialMapping); materialIndex += 2) {
                    if (element->materialIndex == gSignalMaterialMapping[materialIndex + fromIndex]) {
                        element->materialIndex = gSignalMaterialMapping[materialIndex + toIndex];
                        break;
                    }
                }
            }
        }
    }
}