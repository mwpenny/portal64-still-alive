#include "debug_renderer.h"
#include "defs.h"
#include "collision_object.h"

#define SOLID_SHADE_COLOR   0, 0, 0, SHADE, 0, 0, 0, SHADE

Vtx vtx_contact_solver_debug[] = {
    {{{0, 0, -16}, 0, {0, 0}, {255, 0, 0, 255}}},
    {{{0, 0, 16}, 0, {0, 0}, {255, 0, 0, 255}}},
    {{{64, 0, 0}, 0, {0, 0}, {255, 0, 0, 255}}},

    {{{0, -16, 0}, 0, {0, 0}, {0, 255, 0, 255}}},
    {{{64, 0, 0}, 0, {0, 0}, {0, 255, 0, 255}}},
    {{{0, 16, 0}, 0, {0, 0}, {0, 255, 0, 255}}},
};

Gfx contact_solver_debug[] = {
    gsSPVertex(vtx_contact_solver_debug, 6, 0),
    gsSP2Triangles(0, 1, 2, 0, 3, 4, 5, 0),
	gsSPEndDisplayList(),
};

Gfx mat_contact_solver_debug[] = {
    gsDPSetRenderMode(G_RM_OPA_SURF, G_RM_OPA_SURF2),
    gsSPGeometryMode(G_ZBUFFER | G_LIGHTING | G_CULL_BOTH, G_SHADE),
    gsDPSetCombineMode(SOLID_SHADE_COLOR, SOLID_SHADE_COLOR),
	gsSPEndDisplayList(),
};

void contactConstraintStateDebugDraw(struct ContactManifold* constraintState, struct RenderState* renderState) {
    for (int i = 0; i < constraintState->contactCount; ++i) {
        struct ContactPoint* contact = &constraintState->contacts[i];

        float mat[4][4];

        mat[0][0] = constraintState->normal.x;
        mat[0][1] = constraintState->normal.y;
        mat[0][2] = constraintState->normal.z;
        mat[0][3] = 0.0f;

        mat[1][0] = constraintState->tangentVectors[0].x;
        mat[1][1] = constraintState->tangentVectors[0].y;
        mat[1][2] = constraintState->tangentVectors[0].z;
        mat[1][3] = 0.0f;

        mat[2][0] = constraintState->tangentVectors[1].x;
        mat[2][1] = constraintState->tangentVectors[1].y;
        mat[2][2] = constraintState->tangentVectors[1].z;
        mat[2][3] = 0.0f;

        struct Vector3 pos = contact->contactBWorld;

        if (constraintState->shapeB->body) {
            vector3Add(&constraintState->shapeB->body->transform.position, &pos, &pos);
        }

        mat[3][0] = pos.x * SCENE_SCALE;
        mat[3][1] = pos.y * SCENE_SCALE;
        mat[3][2] = pos.z * SCENE_SCALE;
        mat[3][3] = 1.0f;

        Mtx* mtx = renderStateRequestMatrices(renderState, 1);

        if (!mtx) {
            return;
        }

        guMtxF2L(mat, mtx);

        gSPMatrix(renderState->dl++, mtx, G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
        gSPDisplayList(renderState->dl++, contact_solver_debug);
        gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
    }
}

void contactSolverDebugDraw(struct ContactSolver* contactSolver, struct RenderState* renderState) {
    gSPDisplayList(renderState->dl++, mat_contact_solver_debug);
    struct ContactManifold* contact = contactSolver->activeContacts;

    while (contact) {
        contactConstraintStateDebugDraw(contact, renderState);
        contact = contact->next;
    }
}