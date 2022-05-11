#include "dynamic_scene.h"

struct DynamicScene gDynamicScene;

void dynamicSceneInit() {
    for (int i = 0; i < MAX_DYNAMIC_SCENE_OBJECTS; ++i) {
        gDynamicScene.objects[i].flags = 0;
    }
}

void dynamicSceneRender(struct RenderState* renderState, int touchingPortals) {
    int flagMask = DYNAMIC_SCENE_OBJECT_FLAGS_USED | DYNAMIC_SCENE_OBJECT_FLAGS_ACTIVE | DYNAMIC_SCENE_OBJECT_FLAGS_TOUCHING_PORTAL;
    int flagValue = DYNAMIC_SCENE_OBJECT_FLAGS_USED | DYNAMIC_SCENE_OBJECT_FLAGS_ACTIVE;

    if (touchingPortals) {
        flagValue |= DYNAMIC_SCENE_OBJECT_FLAGS_TOUCHING_PORTAL;
    }

    for (int i = 0; i < MAX_DYNAMIC_SCENE_OBJECTS; ++i) {
        if ((gDynamicScene.objects[i].flags & flagMask) == flagValue) {
            gDynamicScene.objects[i].renderCallback(gDynamicScene.objects[i].data, renderState);
        }
    }
}

int dynamicSceneAdd(void* data, DynamicRender renderCallback) {
    for (int i = 0; i < MAX_DYNAMIC_SCENE_OBJECTS; ++i) {
        if (!(gDynamicScene.objects[i].flags & DYNAMIC_SCENE_OBJECT_FLAGS_USED)) {
            gDynamicScene.objects[i].flags = DYNAMIC_SCENE_OBJECT_FLAGS_USED | DYNAMIC_SCENE_OBJECT_FLAGS_ACTIVE;
            gDynamicScene.objects[i].data = data;
            gDynamicScene.objects[i].renderCallback = renderCallback;
            return i;
        }
    }

    return INVALID_DYNAMIC_OBJECT;
}

void dynamicSceneRemove(int id) {
    if (id < 0 || id >= MAX_DYNAMIC_SCENE_OBJECTS) {
        return;
    }

    gDynamicScene.objects[id].flags = 0;
}

void dynamicSceneSetFlags(int id, int flags) {
    gDynamicScene.objects[id].flags |= flags;
}

void dynamicSceneClearFlags(int id, int flags) {
    gDynamicScene.objects[id].flags &= ~flags;

}