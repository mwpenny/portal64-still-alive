#ifndef __DYNAMIC_SCENE_H__
#define __DYNAMIC_SCENE_H__

#include <stdint.h>

#include "graphics/render_scene.h"
#include "graphics/renderstate.h"
#include "math/transform.h"
#include "scene/camera.h"

struct DynamicRenderDataList;

typedef int  (*DynamicCull)(void* data, struct FrustumCullingInformation* frustum);
typedef void (*DynamicRender)(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState);
typedef void (*DynamicViewRender)(void* data, struct RenderScene* renderScene, struct Transform* fromView);

#define MAX_DYNAMIC_SCENE_OBJECTS       64
#define MAX_VIEW_DEPENDENT_OBJECTS      24

#define DYNAMIC_SCENE_OBJECT_FLAGS_USED (1 << 0)
#define DYNAMIC_SCENE_OBJECT_SKIP_ROOT  (1 << 1)

#define INVALID_DYNAMIC_OBJECT          -1

struct DynamicSceneObject {
    void* data;
    struct Vector3* position;
    float scaledRadius;
    DynamicCull preciseCullingCallback;
    union {
        DynamicRender renderCallback;           // For objects
        DynamicViewRender viewRenderCallback;   // For viewDependentObjects
    };
    uint16_t flags;
    uint64_t roomFlags;
};

struct DynamicScene {
    struct DynamicSceneObject objects[MAX_DYNAMIC_SCENE_OBJECTS];
    struct DynamicSceneObject viewDependentObjects[MAX_VIEW_DEPENDENT_OBJECTS];
};

void dynamicSceneInit();

int dynamicSceneAdd(void* data, DynamicRender renderCallback, struct Vector3* position, float radius);
int dynamicSceneAddViewDependent(void* data, DynamicViewRender renderCallback, struct Vector3* position, float radius);
int dynamicSceneObjectCount();
int dynamicSceneViewDependentObjectCount();

void dynamicSceneRemove(int id);
void dynamicSceneSetFlags(int id, int flags);
void dynamicSceneClearFlags(int id, int flags);

void dynamicSceneSetRoomFlags(int id, uint64_t roomFlags);
void dynamicSceneSetPreciseCullingCallback(int id, DynamicCull callback);

void dynamicRenderListAddData(
    struct DynamicRenderDataList* list,
    Gfx* model,
    Mtx* transform,
    short materialIndex,
    struct Vector3* position,
    Mtx* armature
);

void dynamicRenderListAddDataTouchingPortal(
    struct DynamicRenderDataList* list,
    Gfx* model,
    Mtx* transform,
    short materialIndex,
    struct Vector3* position,
    Mtx* armature,
    int rigidBodyFlags
);

#endif
