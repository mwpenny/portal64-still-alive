#include "cube.h"
#include "../physics/collision_box.h"
#include "../models/models.h"
#include "defs.h"

struct CollisionBox gCubeCollisionBox = {
    {0.3165f, 0.3165f, 0.3165f}
};

struct Plane gFloor = {{0.0f, 1.0f, 0.0f}, 0.0f};

struct ContactSolver gContactSolver;

struct ColliderTypeData gCubeCollider = {
    CollisionShapeTypeBox,
    &gCubeCollisionBox,
    0.5f,
    0.5f,
    &gCollisionBoxCallbacks,  
};

void cubeInit(struct Cube* cube) {
    rigidBodyInit(&cube->rigidBody, &gCubeCollider, 1.0f);
}

void cubeUpdate(struct Cube* cube) {
    rigidBodyUpdate(&cube->rigidBody);

    gContactSolver.contactCount = 0;

    rigidBodyCollideWithPlane(&cube->rigidBody, &gFloor, &gContactSolver);

    contactSolverSolve(&gContactSolver);
}

void cubeRender(struct Cube* cube, struct RenderState* renderState) {
    Mtx* matrix = renderStateRequestMatrices(renderState, 1);
    transformToMatrixL(&cube->rigidBody.transform, matrix, SCENE_SCALE);

    gSPMatrix(renderState->dl++, matrix, G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL);
    gSPDisplayList(renderState->dl++, cube_CubeSimpleBevel_mesh);
    gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
}