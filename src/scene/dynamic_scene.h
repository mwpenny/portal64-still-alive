#ifndef __DYNAMIC_SCENE_H__
#define __DYNAMIC_SCENE_H__

#include "../graphics/renderstate.h"

typedef void (*DynamicRender)(void* data, struct RenderState* renderState);

#define MAX_DYNAMIC_SCENE_OBJECTS 32

#define DYNAMIC_SCENE_OBJECT_FLAGS_USED                 (1 << 0)
#define DYNAMIC_SCENE_OBJECT_FLAGS_ACTIVE               (1 << 1)
#define DYNAMIC_SCENE_OBJECT_FLAGS_TOUCHING_PORTAL      (1 << 2)

#define INVALID_DYNAMIC_OBJECT  -1

struct DynamicSceneObject {
    void* data;
    DynamicRender renderCallback;
    int flags;
};

struct DynamicScene {
    struct DynamicSceneObject objects[MAX_DYNAMIC_SCENE_OBJECTS];
};

void dynamicSceneInit();

void dynamicSceneRender(struct RenderState* renderState, int touchingPortals);

int dynamicSceneAdd(void* data, DynamicRender renderCallback);
void dynamicSceneRemove(int id);
void dynamicSceneSetFlags(int id, int flags);
void dynamicSceneClearFlags(int id, int flags);

#endif