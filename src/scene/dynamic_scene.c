#include "dynamic_scene.h"
#include "../levels/static_render.h"
#include "defs.h"
#include "../levels/levels.h"

struct DynamicScene gDynamicScene;

#define FLAG_MASK (DYNAMIC_SCENE_OBJECT_FLAGS_USED | DYNAMIC_SCENE_OBJECT_FLAGS_ACTIVE | DYNAMIC_SCENE_OBJECT_FLAGS_TOUCHING_PORTAL)

#define FLAG_VALUE_NOT_TOUCHING_PORTAL (DYNAMIC_SCENE_OBJECT_FLAGS_USED | DYNAMIC_SCENE_OBJECT_FLAGS_ACTIVE)
#define FLAG_VALUE_TOUCHING_PORTAL (DYNAMIC_SCENE_OBJECT_FLAGS_USED | DYNAMIC_SCENE_OBJECT_FLAGS_ACTIVE | DYNAMIC_SCENE_OBJECT_FLAGS_TOUCHING_PORTAL)

void dynamicSceneInit() {
    for (int i = 0; i < MAX_DYNAMIC_SCENE_OBJECTS; ++i) {
        gDynamicScene.objects[i].flags = 0;
    }
}


void dynamicScenePopulateWithFlags(struct FrustrumCullingInformation* cullingInfo, struct RenderScene* renderScene, int flags) {
    for (int i = 0; i < MAX_DYNAMIC_SCENE_OBJECTS; ++i) {
        struct DynamicSceneObject* object = &gDynamicScene.objects[i];
        if ((object->flags & FLAG_MASK) == flags) {
            struct Vector3 scaledPos;
            vector3Scale(&object->transform->position, &scaledPos, SCENE_SCALE);

            if (isSphereOutsideFrustrum(cullingInfo, &scaledPos, object->scaledRadius)) {
                continue;
            }

            object->renderCallback(object->data, renderScene);
        }
    }
}

void dynamicSceneRenderTouchingPortal(struct Transform* cameraTransform, struct FrustrumCullingInformation* cullingInfo, struct RenderState* renderState) {
    struct RenderScene* tmpScene = renderSceneNew(cameraTransform, renderState, MAX_DYNAMIC_SCENE_OBJECTS, ~0);
    dynamicScenePopulateWithFlags(cullingInfo, tmpScene, FLAG_VALUE_TOUCHING_PORTAL);
    renderSceneGenerate(tmpScene, renderState);
    renderSceneFree(tmpScene);
}

int dynamicSceneAdd(void* data, DynamicRender renderCallback, struct Transform* transform, float radius) {
    for (int i = 0; i < MAX_DYNAMIC_SCENE_OBJECTS; ++i) {
        struct DynamicSceneObject* object = &gDynamicScene.objects[i];
        if (!(object->flags & DYNAMIC_SCENE_OBJECT_FLAGS_USED)) {

            object->flags = DYNAMIC_SCENE_OBJECT_FLAGS_USED | DYNAMIC_SCENE_OBJECT_FLAGS_ACTIVE;
            object->data = data;
            object->renderCallback = renderCallback;
            object->transform = transform;
            object->scaledRadius = radius * SCENE_SCALE;
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

void dynamicScenePopulate(struct FrustrumCullingInformation* cullingInfo, struct RenderScene* renderScene) {
    dynamicScenePopulateWithFlags(cullingInfo, renderScene, FLAG_VALUE_NOT_TOUCHING_PORTAL);
}