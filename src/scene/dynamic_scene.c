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

void dynamicSceneRenderTouchingPortal(struct RenderState* renderState) {
    for (int i = 0; i < MAX_DYNAMIC_SCENE_OBJECTS; ++i) {
        struct DynamicSceneObject* object = &gDynamicScene.objects[i];
        if ((object->flags & FLAG_MASK) == FLAG_VALUE_TOUCHING_PORTAL) {
            if (object->materialIndex != -1) {
                gSPDisplayList(renderState->dl++, levelMaterial(object->materialIndex));
            }

            gDynamicScene.objects[i].renderCallback(gDynamicScene.objects[i].data, renderState);

            if (object->materialIndex != -1) {
                gSPDisplayList(renderState->dl++, levelMaterialRevert(object->materialIndex));
            }
        }
    }
}

int dynamicSceneAdd(void* data, DynamicRender renderCallback, struct Transform* transform, float radius, u16 materialIndex) {
    for (int i = 0; i < MAX_DYNAMIC_SCENE_OBJECTS; ++i) {
        struct DynamicSceneObject* object = &gDynamicScene.objects[i];
        if (!(object->flags & DYNAMIC_SCENE_OBJECT_FLAGS_USED)) {

            object->flags = DYNAMIC_SCENE_OBJECT_FLAGS_USED | DYNAMIC_SCENE_OBJECT_FLAGS_ACTIVE;
            object->data = data;
            object->renderCallback = renderCallback;
            object->transform = transform;
            object->scaledRadius = radius * SCENE_SCALE;
            object->materialIndex = materialIndex;
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

int dynamicScenePopulate(struct FrustrumCullingInformation* cullingInfo, int currentObjectCount, int staticObjectCount, int* sortKey, u16* renderOrder) {
    for (int i = 0; i < MAX_DYNAMIC_SCENE_OBJECTS; ++i) {
        struct DynamicSceneObject* object = &gDynamicScene.objects[i];
        if ((object->flags & FLAG_MASK) == FLAG_VALUE_NOT_TOUCHING_PORTAL) {
            struct Vector3 scaledPos;
            vector3Scale(&object->transform->position, &scaledPos, SCENE_SCALE);

            if (isSphereOutsideFrustrum(cullingInfo, &scaledPos, object->scaledRadius)) {
                continue;
            }

            renderOrder[currentObjectCount] = staticObjectCount + i;
            sortKey[staticObjectCount + i] = staticRenderSorkKeyFromMaterial(
                object->materialIndex, 
                sqrtf(vector3DistSqrd(&scaledPos, &cullingInfo->cameraPosScaled))
            );
            ++currentObjectCount;
        }
    }

    return currentObjectCount;
}

void dynamicSceneRenderObject(int index, struct RenderState* renderState) {
    if (index < 0 || index >= MAX_DYNAMIC_OBJECTS) {
        return;
    }

    struct DynamicSceneObject* object = &gDynamicScene.objects[index];
    object->renderCallback(object->data, renderState);
}

int dynamicSceneObjectMaterialIndex(int objectIndex) {
    if (objectIndex < 0 || objectIndex >= MAX_DYNAMIC_OBJECTS) {
        return -1;
    }

    return gDynamicScene.objects[objectIndex].materialIndex;
}