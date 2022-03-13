#include "shadow_map.h"
#include "util/memory.h"
#include "math/mathf.h"
#include "math/matrix.h"
#include "graphics/graphics.h"
#include "math/plane.h"
#include "defs.h"

#define SHADOW_MAP_WIDTH    64
#define SHADOW_MAP_HEIGHT   64

u16 __attribute__((aligned(64))) shadow_map_buffer[SHADOW_MAP_WIDTH * SHADOW_MAP_HEIGHT];

static Vp shadowMapViewport = {
  .vp = {
    .vscale = {SHADOW_MAP_WIDTH*2, SHADOW_MAP_HEIGHT*2, G_MAXZ/2, 0},	/* scale */
    .vtrans = {SHADOW_MAP_WIDTH*2, SHADOW_MAP_HEIGHT*2, G_MAXZ/2, 0},	/* translate */
  }
};

#define SHADOW_MAP_COMBINE_MODE        0, 0, 0, ENVIRONMENT, 0, 0, 0, ENVIRONMENT

#define SHADOW_PROJECTION_COMBINE_MODE 0, 0, 0, ENVIRONMENT, 0, 0, 0, TEXEL0

Vtx shadowMapVtxData[] = {
    {{{100, 0, 100}, 0, {SHADOW_MAP_WIDTH << 5, 0}, {255, 255, 255, 255}}},
    {{{-100, 0, 100}, 0, {0, 0}, {255, 255, 255, 255}}},
    {{{-100, 0, -100}, 0, {0, SHADOW_MAP_HEIGHT << 5}, {255, 255, 255, 255}}},
    {{{100, 0, -100}, 0, {SHADOW_MAP_WIDTH << 5, SHADOW_MAP_HEIGHT << 5}, {255, 255, 255, 255}}},

    {{{100, 0, 100}, 0, {SHADOW_MAP_WIDTH << 5, 0}, {255, 255, 255, 255}}},
    {{{-100, 0, 100}, 0, {0, 0}, {255, 255, 255, 255}}},
    {{{-100, 0, -100}, 0, {0, SHADOW_MAP_HEIGHT << 5}, {255, 255, 255, 255}}},
    {{{100, 0, -100}, 0, {SHADOW_MAP_WIDTH << 5, SHADOW_MAP_HEIGHT << 5}, {255, 255, 255, 255}}},
};

Gfx shadowMapGfx0[] = {
    gsSPVertex(shadowMapVtxData, 4, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSPEndDisplayList(),
};

Gfx shadowMapGfx1[] = {
    gsSPVertex(shadowMapVtxData + 4, 4, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSPEndDisplayList(),
};

Gfx* shadowMapGfx[] = {shadowMapGfx0, shadowMapGfx1};
Vtx* shadowMapVtx[] = {shadowMapVtxData, &shadowMapVtxData[4]};

struct Vector3 shadowCornerConfig[] = {
    {1.0f, 1.0f, 1.0f},
    {-1.0f, 1.0f, 1.0f},
    {-1.0f, -1.0f, 1.0f},
    {1.0f, -1.0f, 1.0f},
};

Gfx shadowMapMaterial[] = {
    gsDPPipeSync(),
    gsDPSetEnvColor(255, 255, 255, 255),
    gsDPSetCombineMode(SHADOW_MAP_COMBINE_MODE, SHADOW_MAP_COMBINE_MODE),
    gsSPClearGeometryMode(G_LIGHTING | G_ZBUFFER),
    gsSPEndDisplayList(),
};

void shadowMapRenderOntoPlane(struct ShadowMap* shadowMap, struct RenderState* renderState, struct Transform* lightPovTransform, float nearPlane, float projOffset, struct Plane* ontoPlane, unsigned taskIndex) {
    Vtx* currVtx = shadowMapVtx[taskIndex];

    for (unsigned i = 0; i < 4; ++i) {
        struct Vector3 localSpace;
        localSpace.x = projOffset;
        localSpace.y = projOffset;
        localSpace.z = -nearPlane;
        vector3Multiply(&localSpace, &shadowCornerConfig[i], &localSpace);
        transformPoint(lightPovTransform, &localSpace, &localSpace);
        struct Vector3 rayDir;
        vector3Sub(&localSpace, &lightPovTransform->position, &rayDir);
        vector3Normalize(&rayDir, &rayDir);

        float rayDistance = 0.0f;

        if (!planeRayIntersection(ontoPlane, &lightPovTransform->position, &rayDir, &rayDistance)) {
            return;
        }

        struct Vector3 intersectPoint;
        vector3AddScaled(&lightPovTransform->position, &rayDir, rayDistance, &intersectPoint);

        currVtx->v.ob[0] = (short)intersectPoint.x;
        currVtx->v.ob[1] = (short)intersectPoint.y;
        currVtx->v.ob[2] = (short)intersectPoint.z;

        ++currVtx;
    }

    gDPPipeSync(renderState->dl++);
    gDPSetCycleType(renderState->dl++, G_CYC_1CYCLE);
    gDPSetTextureLUT(renderState->dl++, G_TT_NONE);
    gDPSetRenderMode(renderState->dl++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetCombineMode(renderState->dl++, SHADOW_PROJECTION_COMBINE_MODE, SHADOW_PROJECTION_COMBINE_MODE);
    gDPSetEnvColor(renderState->dl++, shadowMap->shadowColor.r, shadowMap->shadowColor.g, shadowMap->shadowColor.b, shadowMap->shadowColor.a);
    gDPTileSync(renderState->dl++);
    gDPLoadTextureTile(
        renderState->dl++,
        shadow_map_buffer,
        G_IM_FMT_I, G_IM_SIZ_8b,
        SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT,
        0, 0,
        SHADOW_MAP_WIDTH-1, SHADOW_MAP_HEIGHT-1,
        0,
        G_TX_CLAMP | G_TX_NOMIRROR, G_TX_CLAMP | G_TX_NOMIRROR,
        0, 0, 
        0, 0
    );

    gSPDisplayList(renderState->dl++, shadowMapGfx[taskIndex]);
}

void shadowMapRender(struct ShadowMap* shadowMap, struct RenderState* renderState, struct GraphicsTask* gfxTask, struct PointLight* from, struct Transform* subjectTransform, struct Plane* onto) {
    struct Vector3 offset;
    vector3Sub(&subjectTransform->position, &from->position, &offset);

    float distance = sqrtf(vector3MagSqrd(&offset));

    float subjectRadius = shadowMap->subjectRadius * subjectTransform->scale.x;

    float nearPlane = distance - subjectRadius;

    if (nearPlane < 0.00001f) {
        return;
    }
    
    float projOffset = nearPlane * subjectRadius / sqrtf(distance * distance - subjectRadius * subjectRadius);

    if (projOffset < 0.00001f) {
        return;
    }

    float projMatrix[4][4];
    u16 perspNorm;
    matrixPerspective(projMatrix, &perspNorm, -projOffset, projOffset, projOffset, -projOffset, nearPlane, distance + subjectRadius);

    struct Transform lightPovTransform;
    lightPovTransform.position = from->position;
    lightPovTransform.scale = gOneVec;
    quatLook(&offset, &gUp, &lightPovTransform.rotation);

    struct Transform povInverse;
    transformInvert(&lightPovTransform, &povInverse);
    float cameraView[4][4];
    transformToMatrix(&povInverse, cameraView, SCENE_SCALE);
    float viewProj[4][4];
    guMtxCatF(cameraView, projMatrix, viewProj);

    float subjectMatrix[4][4];
    transformToMatrix(subjectTransform, subjectMatrix, SCENE_SCALE);
    guMtxCatF(subjectMatrix, viewProj, projMatrix);

    Mtx* lightMtx = renderStateRequestMatrices(renderState, 1);

    guMtxF2L(projMatrix, lightMtx);
    gSPForceMatrix(renderState->dl++, lightMtx);
    gDPPipeSync(renderState->dl++);
    gDPSetCycleType(renderState->dl++, G_CYC_FILL);
    gDPSetColorImage(renderState->dl++, G_IM_FMT_CI, G_IM_SIZ_8b, SHADOW_MAP_WIDTH, osVirtualToPhysical(shadow_map_buffer));
    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, 0, 0, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
    gSPViewport(renderState->dl++, &shadowMapViewport);
    gDPSetFillColor(renderState->dl++, 0);
    gDPFillRectangle(renderState->dl++, 0, 0, SHADOW_MAP_WIDTH-1, SHADOW_MAP_HEIGHT-1);
    gDPPipeSync(renderState->dl++);
    gDPSetCycleType(renderState->dl++, G_CYC_1CYCLE);

    gDPSetRenderMode(renderState->dl++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gSPDisplayList(renderState->dl++, shadowMapMaterial);
    gSPDisplayList(renderState->dl++, shadowMap->subject);

    gDPPipeSync(renderState->dl++);
    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WD, SCREEN_HT);
    gSPViewport(renderState->dl++, &fullscreenViewport);
    gSPSetGeometryMode(renderState->dl++, G_ZBUFFER);

    gDPSetColorImage(renderState->dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WD, osVirtualToPhysical(gfxTask->framebuffer));

    Mtx* identity = renderStateRequestMatrices(renderState, 1);

    guMtxIdent(identity);
    gSPMatrix(renderState->dl++, identity, G_MTX_LOAD | G_MTX_MODELVIEW | G_MTX_NOPUSH);

    shadowMapRenderOntoPlane(shadowMap, renderState, &lightPovTransform, nearPlane, projOffset, onto, gfxTask->taskIndex);
}

#define DEBUG_X 32
#define DEBUG_Y 32

void shadowMapRenderDebug(struct RenderState* renderState) {
    gDPPipeSync(renderState->dl++);
    gDPSetCycleType(renderState->dl++, G_CYC_1CYCLE);
    gDPSetTextureLUT(renderState->dl++, G_TT_NONE);
    gDPSetRenderMode(renderState->dl++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gDPSetTexturePersp(renderState->dl++, G_TP_NONE);
    gDPSetCombineLERP(renderState->dl++, 0, 0, 0, TEXEL0, 0, 0, 0, 1, 0, 0, 0, TEXEL0, 0, 0, 0, 1);
    gDPTileSync(renderState->dl++);
    gDPLoadTextureTile(
        renderState->dl++,
        shadow_map_buffer,
        G_IM_FMT_I, G_IM_SIZ_8b,
        SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT,
        0, 0,
        SHADOW_MAP_WIDTH-1, SHADOW_MAP_HEIGHT-1,
        0,
        G_TX_CLAMP | G_TX_NOMIRROR, G_TX_CLAMP | G_TX_NOMIRROR,
        0, 0, 
        0, 0
    );
    gSPTextureRectangle(
        renderState->dl++, 
        DEBUG_Y << 2, DEBUG_X << 2, 
        (DEBUG_X + SHADOW_MAP_WIDTH) << 2, (DEBUG_Y + SHADOW_MAP_HEIGHT) << 2, 
        G_TX_RENDERTILE, 
        0, 0, 
        1 << 10, 1 << 10
    );
}

void shadowMapInit(struct ShadowMap* shadowMap, Gfx* subject, struct Coloru8 shadowColor) {
    shadowMap->subject = subject;
    shadowMap->subjectRadius = 0.0f;
    shadowMap->shadowColor = shadowColor;

    for (Gfx* curr = subject; GET_GFX_TYPE(curr) != G_ENDDL; ++curr) {
        unsigned type = GET_GFX_TYPE(curr);

        switch (type) {
            case G_VTX:
            {
                Vtx* base = (Vtx*)curr->words.w1;
                unsigned count = _SHIFTR(curr->words.w0,12,8);
                
                for (unsigned i = 0; i < count; ++i) {
                    Vtx_t* curr = &base[i].v;
                    float vtxRadius = (float)curr->ob[0] *  (float)curr->ob[0] +
                        (float)curr->ob[1] *  (float)curr->ob[1] +
                        (float)curr->ob[2] *  (float)curr->ob[2];

                    shadowMap->subjectRadius = MAX(shadowMap->subjectRadius, vtxRadius);
                }
                break;
            }
        }
    }

    shadowMap->subjectRadius = sqrtf(shadowMap->subjectRadius);
}