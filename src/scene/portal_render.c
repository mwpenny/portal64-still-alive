#include "portal_render.h"

#include "../defs.h"
#include "../graphics/graphics.h"
#include "render_plan.h"

#include "../build/assets/models/portal/portal_blue_filled.h"
#include "../build/assets/models/portal/portal_orange_filled.h"

struct Quaternion gVerticalFlip = {0.0f, 1.0f, 0.0f, 0.0f};

void portalRenderScreenCover(struct Vector2s16* points, int pointCount, struct RenderProps* props, struct RenderState* renderState) {
    if (pointCount == 0) {
        return;
    }

    gDPPipeSync(renderState->dl++);
    gDPSetDepthSource(renderState->dl++, G_ZS_PRIM);
    gDPSetPrimDepth(renderState->dl++, 0, 0);
    gDPSetRenderMode(renderState->dl++, CLR_ON_CVG | FORCE_BL | IM_RD | Z_CMP | Z_UPD | CVG_DST_WRAP | ZMODE_OPA | GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA), CLR_ON_CVG | FORCE_BL | IM_RD | Z_CMP | Z_UPD | CVG_DST_WRAP | ZMODE_OPA | GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA));
    gDPSetCombineLERP(renderState->dl++, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE);

    Mtx* idenity = renderStateRequestMatrices(renderState, 1);

    if (!idenity) {
        return;
    }

    guMtxIdent(idenity);

    gSPMatrix(renderState->dl++, idenity, G_MTX_LOAD | G_MTX_MODELVIEW | G_MTX_PUSH);

    Mtx* ortho = renderStateRequestMatrices(renderState, 1);

    if (!ortho) {
        return;
    }

    guOrtho(ortho, 0, SCREEN_WD << 2, 0, SCREEN_HT << 2, -10, 10, 1);

    gSPMatrix(renderState->dl++, ortho, G_MTX_LOAD | G_MTX_PROJECTION | G_MTX_NOPUSH);

    Vtx* vertices = renderStateRequestVertices(renderState, pointCount);

    if (!vertices) {
        return;
    }

    for (int i = 0; i < pointCount; ++i) {
        Vtx* vertex = &vertices[i];
        vertex->v.ob[0] = points[i].x;
        vertex->v.ob[1] = points[i].y;
        vertex->v.ob[2] = 0;

        vertex->v.flag = 0;

        vertex->v.tc[0] = 0;
        vertex->v.tc[1] = 0;
        
        vertex->v.cn[0] = 255;
        vertex->v.cn[1] = 255;
        vertex->v.cn[2] = 255;
        vertex->v.cn[3] = 255;
    }

    gSPVertex(renderState->dl++, vertices, pointCount, 0);

    for (int i = 2; i < pointCount; i += 2) {
        if (i + 1 < pointCount) {
            gSP2Triangles(renderState->dl++, 0, i - 1, i, 0, 0, i, i + 1, 0);
        } else {
            gSP1Triangle(renderState->dl++, 0, i - 1, i, 0);
        }
    }

    gDPPipeSync(renderState->dl++);
    gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
    gSPMatrix(renderState->dl++, props->cameraMatrixInfo.projectionView, G_MTX_LOAD | G_MTX_PROJECTION | G_MTX_NOPUSH);
    gDPSetDepthSource(renderState->dl++, G_ZS_PIXEL);
    gDPSetRenderMode(renderState->dl++, G_RM_ZB_OPA_SURF, G_RM_ZB_OPA_SURF2);
}

void portalDetermineTransform(struct Portal* portal, float portalTransform[4][4]) {
    struct Transform finalTransform;
    finalTransform = portal->transform;

    if (portal->flags & PortalFlagsOddParity) {
        quatMultiply(&portal->transform.rotation, &gVerticalFlip, &finalTransform.rotation);
    }
    
    vector3Scale(&gOneVec, &finalTransform.scale, portal->scale);

    transformToMatrix(&finalTransform, portalTransform, SCENE_SCALE);
}

void portalRenderCover(struct Portal* portal, float portalTransform[4][4], struct RenderState* renderState) {
    Mtx* matrix = renderStateRequestMatrices(renderState, 1);

    if (!matrix) {
        return;
    }

    guMtxF2L(portalTransform, matrix);
    gSPMatrix(renderState->dl++, matrix, G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL);
    if (portal->flags & PortalFlagsOddParity) {
        gSPDisplayList(renderState->dl++, portal_portal_blue_filled_model_gfx);
    } else {
        gSPDisplayList(renderState->dl++, portal_portal_orange_filled_model_gfx);
    }
    gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
}