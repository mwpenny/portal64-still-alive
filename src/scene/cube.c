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

struct Plane gFloor = {{0.0f, 1.0f, 0.0f}, 0.0f};

struct ColliderTypeData gFloorColliderType = {
    CollisionShapeTypeQuad,
    &gFloor,
    0.0f,
    1.0f,
    NULL,
};

struct CollisionObject gFloorObject = {
    &gFloorColliderType,
    NULL,
};

struct CollisionQuad gFloatingQuad = {
    {-1.0f, 0.0f, 0.0f},
    {1.0f, 0.0f, 0.0f},
    2.0f,
    {0.0f, 1.0f, 0.0f},
    2.0f,
    {{0.0f, 0.0f, -1.0}, 0.0f},
    0xF,
};

struct ColliderTypeData gFloatingQuadCollider = {
    CollisionShapeTypeQuad,
    &gFloatingQuad,
    0.0f,
    1.0f,
    NULL,
} ;


struct CollisionObject gFloatingQuadObject = {
    &gFloatingQuadCollider,
    NULL,
};

struct ColliderTypeData gCubeCollider = {
    CollisionShapeTypeBox,
    &gCubeCollisionBox,
    0.5f,
    0.5f,
    &gCollisionBoxCallbacks,  
};

void cubeRender(void* data, struct RenderScene* renderScene) {
    struct Cube* cube = (struct Cube*)data;
    Mtx* matrix = renderStateRequestMatrices(renderScene->renderState, 1);
    transformToMatrixL(&cube->rigidBody.transform, matrix, SCENE_SCALE);

    renderSceneAdd(renderScene, cube_gfx, matrix, (cube->rigidBody.flags & RigidBodyDebugFlag) ? button_material_index : cube_material_index, &cube->rigidBody.transform.position);
}

void cubeInit(struct Cube* cube) {
    collisionObjectInit(&cube->collisionObject, &gCubeCollider, &cube->rigidBody, 1.0f);
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
