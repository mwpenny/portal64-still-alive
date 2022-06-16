#include "cube.h"
#include "../physics/collision_box.h"
#include "../models/models.h"
#include "defs.h"
#include "../graphics/debug_render.h"
#include "levels/levels.h"
#include "physics/collision_scene.h"
#include "dynamic_scene.h"

struct CollisionBox gCubeCollisionBox = {
    {0.3165f, 0.3165f, 0.3165f}
};

struct ColliderTypeData gCubeCollider = {
    CollisionShapeTypeBox,
    &gCubeCollisionBox,
    0.0f,
    0.5f,
    &gCollisionBoxCallbacks,  
};

void cubeRender(void* data, struct RenderScene* renderScene) {
    struct Cube* cube = (struct Cube*)data;
    Mtx* matrix = renderStateRequestMatrices(renderScene->renderState, 1);
    transformToMatrixL(&cube->rigidBody.transform, matrix, SCENE_SCALE);

    renderSceneAdd(renderScene, cube_gfx, matrix, cube_material_index, &cube->rigidBody.transform.position, NULL);
}

void cubeInit(struct Cube* cube) {
    collisionObjectInit(&cube->collisionObject, &gCubeCollider, &cube->rigidBody, 2.0f, COLLISION_LAYERS_TANGIBLE | COLLISION_LAYERS_GRABBABLE | COLLISION_LAYERS_FIZZLER);
    collisionSceneAddDynamicObject(&cube->collisionObject);

    cube->collisionObject.body->flags |= RigidBodyFlagsGrabbable;

    cube->dynamicId = dynamicSceneAdd(cube, cubeRender, &cube->rigidBody.transform, sqrtf(vector3MagSqrd(&gCubeCollisionBox.sideLength)));
}

void cubeUpdate(struct Cube* cube) {
    if (cube->rigidBody.flags & (RigidBodyIsTouchingPortal | RigidBodyWasTouchingPortal)) {
        dynamicSceneSetFlags(cube->dynamicId, DYNAMIC_SCENE_OBJECT_FLAGS_TOUCHING_PORTAL);
    } else {
        dynamicSceneClearFlags(cube->dynamicId, DYNAMIC_SCENE_OBJECT_FLAGS_TOUCHING_PORTAL);
    }
}
