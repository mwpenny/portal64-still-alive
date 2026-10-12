#include "dynamic_scene.h"

#include "levels/levels.h"
#include "levels/static_render.h"

struct DynamicScene gDynamicScene;

void dynamicSceneInit() {
    for (int i = 0; i < MAX_DYNAMIC_SCENE_OBJECTS; ++i) {
        gDynamicScene.objects[i].flags = 0;
    }

    for (int i = 0; i < MAX_VIEW_DEPENDENT_OBJECTS; ++i) {
        gDynamicScene.viewDependentObjects[i].flags = 0;
    }
}

int dynamicSceneAdd(void* data, DynamicRender renderCallback, struct Vector3* position, float radius) {
    for (int i = 0; i < MAX_DYNAMIC_SCENE_OBJECTS; ++i) {
        struct DynamicSceneObject* object = &gDynamicScene.objects[i];
        if (!(object->flags & DYNAMIC_SCENE_OBJECT_FLAGS_USED)) {

            object->flags = DYNAMIC_SCENE_OBJECT_FLAGS_USED;
            object->data = data;
            object->position = position;
            object->scaledRadius = radius * SCENE_SCALE;
            object->preciseCullingCallback = NULL;
            object->renderCallback = renderCallback;
            object->roomFlags = ~0;
            return i;
        }
    }

    return INVALID_DYNAMIC_OBJECT;
}

int dynamicSceneAddViewDependent(void* data, DynamicViewRender renderCallback, struct Vector3* position, float radius) {
    for (int i = 0; i < MAX_VIEW_DEPENDENT_OBJECTS; ++i) {
        struct DynamicSceneObject* object = &gDynamicScene.viewDependentObjects[i];
        if (!(object->flags & DYNAMIC_SCENE_OBJECT_FLAGS_USED)) {

            object->flags = DYNAMIC_SCENE_OBJECT_FLAGS_USED;
            object->data = data;
            object->position = position;
            object->scaledRadius = radius * SCENE_SCALE;
            object->preciseCullingCallback = NULL;
            object->viewRenderCallback = renderCallback;
            object->roomFlags = ~0;
            return i + MAX_DYNAMIC_SCENE_OBJECTS;
        }
    }

    return INVALID_DYNAMIC_OBJECT;
}

int dynamicSceneObjectCount() {
    int count = 0;

    for (int i = 0; i < MAX_DYNAMIC_SCENE_OBJECTS; ++i) {
        if (gDynamicScene.objects[i].flags & DYNAMIC_SCENE_OBJECT_FLAGS_USED) {
            ++count;
        }
    }

    return count;
}

int dynamicSceneViewDependentObjectCount() {
    int count = 0;

    for (int i = 0; i < MAX_VIEW_DEPENDENT_OBJECTS; ++i) {
        if (gDynamicScene.viewDependentObjects[i].flags & DYNAMIC_SCENE_OBJECT_FLAGS_USED) {
            ++count;
        }
    }

    return count;
}

static struct DynamicSceneObject* dynamicSceneGetObject(int id) {
    if (id < 0) {
        return NULL;
    }

    if (id < MAX_DYNAMIC_SCENE_OBJECTS) {
        return &gDynamicScene.objects[id];
    }

    id -= MAX_DYNAMIC_SCENE_OBJECTS;
    if (id < MAX_VIEW_DEPENDENT_OBJECTS) {
        return &gDynamicScene.viewDependentObjects[id];
    }

    return NULL;
}

void dynamicSceneRemove(int id) {
    struct DynamicSceneObject* object = dynamicSceneGetObject(id);
    if (object == NULL) {
        return;
    }

    object->flags = 0;
}

void dynamicSceneSetFlags(int id, int flags) {
    struct DynamicSceneObject* object = dynamicSceneGetObject(id);
    if (object == NULL) {
        return;
    }

    object->flags |= flags;
}

void dynamicSceneClearFlags(int id, int flags) {
    struct DynamicSceneObject* object = dynamicSceneGetObject(id);
    if (object == NULL) {
        return;
    }

    object->flags &= ~flags;
}

void dynamicSceneSetRoomFlags(int id, uint64_t roomFlags) {
    struct DynamicSceneObject* object = dynamicSceneGetObject(id);
    if (object == NULL) {
        return;
    }

    object->roomFlags = roomFlags;
}

void dynamicSceneSetPreciseCullingCallback(int id, DynamicCull callback) {
    struct DynamicSceneObject* object = dynamicSceneGetObject(id);
    if (object == NULL) {
        return;
    }

    object->preciseCullingCallback = callback;
}
