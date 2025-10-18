#include "static_render.h"

#include "defs.h"
#include "graphics/render_scene.h"
#include "levels.h"
#include "math/mathf.h"
#include "math/rotated_box.h"
#include "scene/signals.h"
#include "util/memory.h"

#include "codegen/assets/materials/static.h"

void staticRenderTraverseIndex(
    struct StaticContentBox* box,
    struct StaticContentBox* boxEnd, 
    struct StaticContentElement* staticContent, 
    struct RotatedBox* staticBoundingBoxes,
    struct FrustumCullingInformation* cullingInfo,
    struct RenderScene* renderScene
) {
    struct StaticContentBox* fullyVisibleEnd = box;    

    while (box < boxEnd) {
        if (box >= fullyVisibleEnd) {
            enum FrustumResult cullResult = isOutsideFrustum(cullingInfo, &box->box);

            if (cullResult == FrustumResultOutisde) {
                // skip all children
                box = box + box->siblingOffset;
                continue;
            } else if (cullResult == FrustumResultInside) {
                fullyVisibleEnd = box + box->siblingOffset;
            }
        }

        // Leaf nodes
        for (int i = box->staticRange.min; i < box->staticRange.max; ++i) {
            if (staticContent[i].boundingBoxIndex != NO_BOUNDING_BOX_INDEX &&
                isRotatedBoxOutsideFrustum(cullingInfo, &staticBoundingBoxes[staticContent[i].boundingBoxIndex])) {

                continue;
            }

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

#define ANIMATED_CULL_THRESHOLD     50

void staticRenderPopulateRooms(struct FrustumCullingInformation* cullingInfo, Mtx* staticMatrices, struct Transform* staticTransforms, struct RenderScene* renderScene) {
    int currentRoom = 0;

    u64 visibleRooms = renderScene->visibleRooms;

    while (visibleRooms) {
        if (0x1 & visibleRooms) {
            struct StaticIndex* roomIndex = &gCurrentLevel->roomBvhList[currentRoom];
    
            staticRenderTraverseIndex(
                roomIndex->boxIndex,
                roomIndex->boxIndex + roomIndex->boxCount,
                gCurrentLevel->staticContent,
                gCurrentLevel->staticBoundingBoxes,
                cullingInfo,
                renderScene
            );

            struct BoundingBoxs16* animatedBox = roomIndex->animatedBoxes;

            // For rooms with with many animated elements, calculating whether
            // or not to cull them is more expensive than just rendering them
            short animatedElementCount = roomIndex->animatedRange.max - roomIndex->animatedRange.min;
            u8 shouldCull = animatedElementCount < ANIMATED_CULL_THRESHOLD;

            for (int i = roomIndex->animatedRange.min; i < roomIndex->animatedRange.max; ++i, ++animatedBox) {
                struct StaticContentElement* staticElement = &gCurrentLevel->staticContent[i];

                struct Transform* transform = &staticTransforms[staticElement->transformIndex];

                if (shouldCull) {
                    struct RotatedBox rotatedBox;
                    rotatedBoxTransform(transform, animatedBox, &rotatedBox);

                    if (isRotatedBoxOutsideFrustum(cullingInfo, &rotatedBox)) {
                        continue;
                    }
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

void staticRenderDetermineVisibleRooms(struct FrustumCullingInformation* rootCullingInfo, struct FrustumCullingInformation* cullingInfo, u16 currentRoom, u64* visitedRooms, u64 nonVisibleRooms) {
    if (currentRoom == RIGID_BODY_NO_ROOM) {
        return;
    }

    u64 roomMask = 1LL << currentRoom;
    *visitedRooms |= roomMask;

    nonVisibleRooms |= gCurrentLevel->world.rooms[currentRoom].nonVisibleRooms;

    for (int i = 0; i < gCurrentLevel->world.rooms[currentRoom].doorwayCount; ++i) {
        struct Doorway* doorway = &gCurrentLevel->world.doorways[gCurrentLevel->world.rooms[currentRoom].doorwayIndices[i]];

        if ((doorway->flags & DoorwayFlagsOpen) == 0) {
            continue;
        }

        int newRoom = currentRoom == doorway->roomA ? doorway->roomB : doorway->roomA;
        u64 newRoomMask = 1LL << newRoom;
        if ((*visitedRooms & newRoomMask) || (nonVisibleRooms & newRoomMask)) {
            continue;
        }

        float doorwayDistance = planePointDistance(&doorway->quad.plane, &cullingInfo->cameraPos);

        if (
            // if the player is close enough to the doorway it should still render it, even if facing the wrong way
            (fabsf(doorwayDistance) > FORCE_RENDER_DOORWAY_DISTANCE || collisionQuadDetermineEdges(&cullingInfo->cameraPos, &doorway->quad)) && 
            (isQuadOutsideFrustum(cullingInfo, &doorway->quad) || isQuadOutsideFrustum(rootCullingInfo, &doorway->quad))
         ) {
            continue;
        }

        // Narrow the view to what can be seen through the doorway.
        //
        // This can be improved by first clipping the quad to the current
        // frustum and using the clipped quad to generate the new frustum.
        // Need to measure performance to see if it's worth it.
        struct FrustumCullingInformation doorwayFrustum;
        frustumFromQuad(&cullingInfo->cameraPos, &doorway->quad, &doorwayFrustum);

        staticRenderDetermineVisibleRooms(rootCullingInfo, &doorwayFrustum, newRoom, visitedRooms, nonVisibleRooms);
    };
}

int staticRenderIsRoomVisible(u64 visibleRooms, u16 roomIndex) {
    return (visibleRooms & (1LL << roomIndex)) != 0;
}

void staticRender(struct RenderProps* renderStage, struct DynamicRenderDataList* dynamicList, int stageIndex, Mtx* staticMatrices, struct Transform* staticTransforms, struct RenderState* renderState) {
    if (!gCurrentLevel) {
        return;
    }

    struct Transform* cameraTransform = &renderStage->camera.transform;
    struct FrustumCullingInformation* cullingInfo = &renderStage->cameraMatrixInfo.cullingInformation;
    u64* visibleRooms = &renderStage->visiblerooms;

    struct RenderScene* renderScene = renderSceneNew(cameraTransform, renderState, MAX_RENDER_COUNT, *visibleRooms);

    staticRenderPopulateRooms(cullingInfo, staticMatrices, staticTransforms, renderScene);
    dynamicRenderPopulateRenderScene(dynamicList, stageIndex, renderScene, cameraTransform, cullingInfo, *visibleRooms);
    renderSceneGenerate(renderScene, renderState);

    renderStage->renderPartCount = renderScene->currentRenderPart;

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