#include "door.h"

#include "../graphics/render_scene.h"
#include "defs.h"
#include "../models/models.h"
#include "../scene/dynamic_scene.h"

void doorRender(void* data, struct RenderScene* renderScene) {
    struct Door* door = (struct Door*)data;
    Mtx* matrix = renderStateRequestMatrices(renderScene->renderState, 1);
    transformToMatrixL(&door->rigidBody.transform, matrix, SCENE_SCALE);
    renderSceneAdd(renderScene, door_01_gfx, matrix, door_01_material_index, &door->rigidBody.transform.position, NULL);
}

void doorInit(struct Door* door, struct Transform* at, int roomA, int roomB, int doorwayIndex) {
    // collisionObjectInit(&cube->collisionObject, &gCubeCollider, &cube->rigidBody, 1.0f);
    // collisionSceneAddDynamicObject(&cube->collisionObject);

    // cube->collisionObject.body->flags |= RigidBodyFlagsGrabbable;

    door->rigidBody.transform = *at;

    door->dynamicId = dynamicSceneAdd(door, doorRender, &door->rigidBody.transform, 1.7f);
}