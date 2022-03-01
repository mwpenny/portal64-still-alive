#include "portal.h"

#include "models/models.h"
#include "../graphics/screen_clipper.h"
#include "../graphics/graphics.h"
#include "../defs.h"

#define CALC_SCREEN_SPACE(clip_space, screen_size) ((clip_space + 1.0f) * ((screen_size) / 2))

struct Vector3 gPortalOutline[] = {
    {0.0f, 1.0f * SCENE_SCALE, 0.0f},
    {0.353553f * SCENE_SCALE, 0.707107f * SCENE_SCALE, 0.0f},
    {0.5f * SCENE_SCALE, 0.0f, 0.0f},
    {0.353553f * SCENE_SCALE, -0.707107f * SCENE_SCALE, 0.0f},
    {0.0f, -1.0f * SCENE_SCALE, 0.0f},
    {-0.353553f * SCENE_SCALE, -0.707107f * SCENE_SCALE, 0.0f},
    {-0.5f * SCENE_SCALE, 0.0f, 0.0f},
    {-0.353553f * SCENE_SCALE, 0.707107f * SCENE_SCALE, 0.0f},
};

void portalInit(struct Portal* portal, enum PortalFlags flags) {
    transformInitIdentity(&portal->transform);
    portal->flags = flags;
}

void portalRender(struct Portal* portal, struct Portal* otherPortal, struct Camera* camera, struct RenderState* renderState) {
    struct ScreenClipper clipper;
    float portalTransform[4][4];

    transformToMatrix(&portal->transform, portalTransform);

    screenClipperInitWithCamera(&clipper, camera, (float)SCREEN_WD / (float)SCREEN_HT, portalTransform);

    struct Box2D clippingBounds;

    screenClipperBoundingPoints(&clipper, gPortalOutline, sizeof(gPortalOutline) / sizeof(*gPortalOutline), &clippingBounds);

    Mtx* matrix = renderStateRequestMatrices(renderState, 1);

    guMtxF2L(portalTransform, matrix);
    gSPMatrix(renderState->dl++, matrix, G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL);
    gSPDisplayList(renderState->dl++, portal_mask_Circle_mesh);
    gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);

    if (clippingBounds.min.x < clippingBounds.max.x) {
        int minX = CALC_SCREEN_SPACE(clippingBounds.min.x, SCREEN_WD << 2);
        int maxX = CALC_SCREEN_SPACE(clippingBounds.max.x, SCREEN_WD << 2);
        int minY = CALC_SCREEN_SPACE(-clippingBounds.max.y, SCREEN_HT << 2);
        int maxY = CALC_SCREEN_SPACE(-clippingBounds.min.y, SCREEN_HT << 2);

        gDPPipeSync(renderState->dl++);
        gDPSetCombineLERP(renderState->dl++, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE);
        gDPSetRenderMode(renderState->dl++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
        gDPSetPrimColor(renderState->dl++, 0, 0, 0, 255, 0, 64);
        gSPTextureRectangle(renderState->dl++, minX, minY, maxX, maxY, 0, 0, 0, 1 << 10, 1 << 10);
        gDPPipeSync(renderState->dl++);
    }
}