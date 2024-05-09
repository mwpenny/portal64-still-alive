#include "debug_render.h"

#include <ultra64.h>

#include "defs.h"
#include "../math/vector3.h"
#include "../math/matrix.h"

Vtx vtx_quad[] = {
    {{{0, 0, 0}, 0, {0, 0}, {200, 0, 0, 255}}},
    {{{SCENE_SCALE, 0, 0}, 0, {0, 0}, {200, 0, 0, 255}}},
    {{{SCENE_SCALE, SCENE_SCALE, 0}, 0, {0, 0}, {200, 0, 0, 255}}},
    {{{0, SCENE_SCALE, 0}, 0, {0, 0}, {200, 0, 0, 255}}},
};

#define SOLID_SHADE_COLOR   0, 0, 0, SHADE, 0, 0, 0, SHADE

Gfx mat_quad[] = {
    gsDPSetRenderMode(G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2),
    gsSPGeometryMode(G_LIGHTING | G_CULL_BOTH, G_ZBUFFER | G_SHADE),
    gsDPSetCombineMode(SOLID_SHADE_COLOR, SOLID_SHADE_COLOR),
	gsSPEndDisplayList(),
};

Gfx gfx_quad[] = {
    gsSPVertex(vtx_quad, 4, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
	gsSPEndDisplayList(),
};

void matrixFromBasisL(Mtx* matrix, struct Vector3* origin, struct Vector3* x, struct Vector3* y, struct Vector3* z) {
    float fmtx[4][4];
    matrixFromBasis(fmtx, origin, x, y, z);
    guMtxF2L(fmtx, matrix);
}

void debugRenderQuad(struct Vector3* origin, struct Vector3* edgeA, struct Vector3* edgeB, float edgeLengthA, float edgeLengthB, struct RenderState* renderState) {
    Mtx* mtx = renderStateRequestMatrices(renderState, 1);

    if (!mtx) {
        return;
    }
    
    struct Vector3 normal;
    struct Vector3 x;
    struct Vector3 y;
    vector3Scale(edgeA, &x, edgeLengthA);
    vector3Scale(edgeB, &y, edgeLengthB);
    vector3Cross(edgeA, edgeB, &normal);
    matrixFromBasisL(mtx, origin, &x, &y, &normal);

    gSPMatrix(renderState->dl++, mtx, G_MTX_MUL | G_MTX_PUSH | G_MTX_MODELVIEW);
    gSPDisplayList(renderState->dl++, mat_quad);
    gSPDisplayList(renderState->dl++, gfx_quad);
    gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
}