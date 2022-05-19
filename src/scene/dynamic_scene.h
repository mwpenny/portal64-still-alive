#ifndef __DYNAMIC_SCENE_H__
#define __DYNAMIC_SCENE_H__

#include "../graphics/renderstate.h"
#include "../math/transform.h"
#include "../scene/camera.h"

typedef void (*DynamicRender)(void* data, struct RenderState* renderState);

#define MAX_DYNAMIC_SCENE_OBJECTS 32

#define DYNAMIC_SCENE_OBJECT_FLAGS_USED                 (1 << 0)
#define DYNAMIC_SCENE_OBJECT_FLAGS_ACTIVE               (1 << 1)
#define DYNAMIC_SCENE_OBJECT_FLAGS_TOUCHING_PORTAL      (1 << 2)

#define INVALID_DYNAMIC_OBJECT  -1

struct DynamicSceneObject {
    void* data;
    DynamicRender renderCallback;
    struct Transform* transform;
    float scaledRadius;
    u16 materialIndex;
    u16 flags;
};

struct DynamicScene {
    struct DynamicSceneObject objects[MAX_DYNAMIC_SCENE_OBJECTS];
};

void dynamicSceneInit();

void dynamicSceneRenderTouchingPortal(struct RenderState* renderState);

int dynamicSceneAdd(void* data, DynamicRender renderCallback, struct Transform* transform, float radius, u16 materialIndex);
void dynamicSceneRemove(int id);
void dynamicSceneSetFlags(int id, int flags);
void dynamicSceneClearFlags(int id, int flags);

int dynamicScenePopulate(struct FrustrumCullingInformation* cullingInfo, int currentObjectCount, int staticObjectCount, int* sortKey, u16* renderOrder);

void dynamicSceneRenderObject(int index, struct RenderState* renderState);
int dynamicSceneObjectMaterialIndex(int objectIndex);

#endif