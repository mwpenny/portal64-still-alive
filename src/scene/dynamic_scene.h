#ifndef __DYNAMIC_SCENE_H__
#define __DYNAMIC_SCENE_H__

#include "../graphics/renderstate.h"
#include "../math/transform.h"
#include "../scene/camera.h"
#include "../graphics/render_scene.h"

typedef void (*DynamicRender)(void* data, struct RenderScene* renderScene);

#define MAX_DYNAMIC_SCENE_OBJECTS 32

#define DYNAMIC_SCENE_OBJECT_FLAGS_USED                 (1 << 0)
#define DYNAMIC_SCENE_OBJECT_FLAGS_ACTIVE               (1 << 1)

#define INVALID_DYNAMIC_OBJECT  -1

struct DynamicSceneObject {
    void* data;
    DynamicRender renderCallback;
    struct Transform* transform;
    float scaledRadius;
    u16 flags;
};

struct DynamicScene {
    struct DynamicSceneObject objects[MAX_DYNAMIC_SCENE_OBJECTS];
};

void dynamicSceneInit();

int dynamicSceneAdd(void* data, DynamicRender renderCallback, struct Transform* transform, float radius);
void dynamicSceneRemove(int id);
void dynamicSceneSetFlags(int id, int flags);
void dynamicSceneClearFlags(int id, int flags);

void dynamicScenePopulate(struct FrustrumCullingInformation* cullingInfo, struct RenderScene* renderScene);

#endif