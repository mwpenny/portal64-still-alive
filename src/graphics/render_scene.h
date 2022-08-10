#ifndef __RENDER_SCENE_H__
#define __RENDER_SCENE_H__

#include <ultra64.h>
#include "../math/transform.h"
#include "../math/plane.h"
#include "renderstate.h"

struct RenderPart {
    Mtx* matrix;
    Gfx* geometry;
    Mtx* armature;
};

struct RenderScene {
    u64 visibleRooms;
    struct Plane forwardPlane;
    struct RenderPart* renderParts;
    short* materials;
    int* sortKeys;
    short* renderOrder;
    short* renderOrderCopy;
    int currentRenderPart;
    int maxRenderParts;
    struct RenderState *renderState;
};

struct RenderScene* renderSceneNew(struct Transform* cameraTransform, struct RenderState *renderState, int capacity, u64 visibleRooms);
void renderSceneFree(struct RenderScene* renderScene);
void renderSceneAdd(struct RenderScene* renderScene, Gfx* geometry, Mtx* matrix, int materialIndex, struct Vector3* at, Mtx* armature);
void renderSceneGenerate(struct RenderScene* renderScene, struct RenderState* renderState);

#define RENDER_SCENE_IS_ROOM_VISIBLE(renderScene, roomIndex) (((renderScene)->visibleRooms & (1 << (roomIndex))) != 0)

#endif