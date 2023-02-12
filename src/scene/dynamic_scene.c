#include "dynamic_scene.h"
#include "../levels/static_render.h"
#include "defs.h"
#include "../levels/levels.h"

struct DynamicScene gDynamicScene;

void dynamicSceneInit() {
    for (int i = 0; i < MAX_DYNAMIC_SCENE_OBJECTS; ++i) {
        gDynamicScene.objects[i].flags = 0;
    }

    for (int i = 0; i < MAX_VIEW_DEPENDANT_OBJECTS; ++i) {
        gDynamicScene.viewDependantObjects[i].flags = 0;
    }
}

int dynamicSceneAdd(void* data, DynamicRender renderCallback, struct Vector3* position, float radius) {
    for (int i = 0; i < MAX_DYNAMIC_SCENE_OBJECTS; ++i) {
        struct DynamicSceneObject* object = &gDynamicScene.objects[i];
        if (!(object->flags & DYNAMIC_SCENE_OBJECT_FLAGS_USED)) {

            object->flags = DYNAMIC_SCENE_OBJECT_FLAGS_USED | DYNAMIC_SCENE_OBJECT_FLAGS_ACTIVE;
            object->data = data;
            object->renderCallback = renderCallback;
            object->position = position;
            object->scaledRadius = radius * SCENE_SCALE;
            object->roomFlags = ~0;
            return i;
        }
    }

    return INVALID_DYNAMIC_OBJECT;
}

int dynamicSceneAddViewDependant(void* data, DynamicViewRender renderCallback, struct Vector3* position, float radius) {
    for (int i = 0; i < MAX_VIEW_DEPENDANT_OBJECTS; ++i) {
        struct DynamicSceneViewDependantObject* object = &gDynamicScene.viewDependantObjects[i];
        if (!(object->flags & DYNAMIC_SCENE_OBJECT_FLAGS_USED)) {

            object->flags = DYNAMIC_SCENE_OBJECT_FLAGS_USED | DYNAMIC_SCENE_OBJECT_FLAGS_ACTIVE;
            object->data = data;
            object->renderCallback = renderCallback;
            object->position = position;
            object->scaledRadius = radius * SCENE_SCALE;
            object->roomFlags = ~0;
            return i + MAX_DYNAMIC_SCENE_OBJECTS;
        }
    }

    return INVALID_DYNAMIC_OBJECT;
}

void dynamicSceneRemove(int id) {
    if (id < 0) {
        return;
    }

    if (id < MAX_DYNAMIC_SCENE_OBJECTS) {
        gDynamicScene.objects[id].flags = 0;
    }

    id -= MAX_DYNAMIC_SCENE_OBJECTS;

    if (id < MAX_VIEW_DEPENDANT_OBJECTS) {
        gDynamicScene.viewDependantObjects[id].flags = 0;
    }
}

void dynamicSceneSetFlags(int id, int flags) {
    if (id < 0) {
        return;
    }

    if (id < MAX_DYNAMIC_SCENE_OBJECTS) {
        gDynamicScene.objects[id].flags |= flags;
    }

    id -= MAX_DYNAMIC_SCENE_OBJECTS;

    if (id < MAX_VIEW_DEPENDANT_OBJECTS) {
        gDynamicScene.viewDependantObjects[id].flags |= flags;
    }
}

void dynamicSceneClearFlags(int id, int flags) {
    if (id < 0) {
        return;
    }

    if (id < MAX_DYNAMIC_SCENE_OBJECTS) {
        gDynamicScene.objects[id].flags &= ~flags;
    }

    id -= MAX_DYNAMIC_SCENE_OBJECTS;

    if (id < MAX_VIEW_DEPENDANT_OBJECTS) {
        gDynamicScene.viewDependantObjects[id].flags &= ~flags;
    }
}

void dynamicSceneSetRoomFlags(int id, u64 roomFlags) {
    if (id < 0) {
        return;
    }

    if (id < MAX_DYNAMIC_SCENE_OBJECTS) {
        gDynamicScene.objects[id].roomFlags = roomFlags;
    }

    id -= MAX_DYNAMIC_SCENE_OBJECTS;

    if (id < MAX_VIEW_DEPENDANT_OBJECTS) {
        gDynamicScene.viewDependantObjects[id].roomFlags = roomFlags;
    }
}