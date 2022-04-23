#include "cube.h"
#include "../physics/collision_box.h"
#include "../models/models.h"
#include "defs.h"
#include "../graphics/debug_render.h"
#include "levels/levels.h"
#include "physics/collision_scene.h"

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

void cubeInit(struct Cube* cube) {
    collisionObjectInit(&cube->collisionObject, &gCubeCollider, &cube->rigidBody, 1.0f);
    collisionSceneAddDynamicObject(&cube->collisionObject);

    cube->collisionObject.body->flags |= RigidBodyFlagsGrabbable;
}

void cubeUpdate(struct Cube* cube) {
    
}

void cubeRender(struct Cube* cube, struct RenderState* renderState) {
    Mtx* matrix = renderStateRequestMatrices(renderState, 1);
    transformToMatrixL(&cube->rigidBody.transform, matrix, SCENE_SCALE);

    gSPMatrix(renderState->dl++, matrix, G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL);
    gSPDisplayList(renderState->dl++, cube_CubeSimpleBevel_mesh);
    gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);

    debugRenderQuad(&gFloatingQuad.corner, &gFloatingQuad.edgeA, &gFloatingQuad.edgeB, gFloatingQuad.edgeALength, gFloatingQuad.edgeBLength, renderState);
}