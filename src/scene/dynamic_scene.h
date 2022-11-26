#ifndef __DYNAMIC_SCENE_H__
#define __DYNAMIC_SCENE_H__

#include "../graphics/renderstate.h"
#include "../math/transform.h"
#include "../scene/camera.h"
#include "../graphics/render_scene.h"

struct DynamicRenderDataList;

typedef void (*DynamicRender)(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState);

#define MAX_DYNAMIC_SCENE_OBJECTS 32

#define DYNAMIC_SCENE_OBJECT_FLAGS_USED                 (1 << 0)
#define DYNAMIC_SCENE_OBJECT_FLAGS_ACTIVE               (1 << 1)
#define DYNAMIC_SCENE_OBJECT_SKIP_ROOT                  (1 << 2)

#define INVALID_DYNAMIC_OBJECT  -1

#define ROOM_FLAG_FROM_INDEX(flag) (1 << (flag))

struct DynamicSceneObject {
    void* data;
    DynamicRender renderCallback;
    struct Transform* transform;
    float scaledRadius;
    u16 flags;
    u64 roomFlags;
};

struct DynamicScene {
    struct DynamicSceneObject objects[MAX_DYNAMIC_SCENE_OBJECTS];
};

void dynamicSceneInit();

int dynamicSceneAdd(void* data, DynamicRender renderCallback, struct Transform* transform, float radius);
void dynamicSceneRemove(int id);
void dynamicSceneSetFlags(int id, int flags);
void dynamicSceneClearFlags(int id, int flags);

void dynamicSceneSetRoomFlags(int id, u64 roomFlags);

void dynamicRenderListAddData(
    struct DynamicRenderDataList* list,
    Gfx* model,
    Mtx* transform,
    short materialIndex,
    struct Vector3* position,
    Mtx* armature
);

#endif