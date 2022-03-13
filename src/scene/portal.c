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

struct Quaternion gVerticalFlip = {0.0f, 1.0f, 0.0f, 0.0f};

void renderPropsInit(struct RenderProps* props, struct Camera* camera, float aspectRatio, struct RenderState* renderState) {
    props->camera = *camera;
    props->aspectRatio = aspectRatio;
    props->perspectiveMatrix = cameraSetupMatrices(camera, renderState, aspectRatio, &props->perspectiveCorrect);
    props->currentDepth = STARTING_RENDER_DEPTH;

    props->minX = 0;
    props->minY = 0;
    props->maxX = SCREEN_WD;
    props->maxY = SCREEN_HT;
}

void renderPropsNext(struct RenderProps* current, struct RenderProps* next, struct Transform* fromPortal, struct Transform* toPortal, short minX, short minY, short maxX, short maxY, struct RenderState* renderState) {
    struct Transform otherInverse;
    transformInvert(fromPortal, &otherInverse);
    struct Transform portalCombined;
    transformConcat(toPortal, &otherInverse, &portalCombined);

    next->camera = current->camera;
    next->aspectRatio = current->aspectRatio;
    transformConcat(&portalCombined, &current->camera.transform, &next->camera.transform);

    next->perspectiveMatrix = cameraSetupMatrices(&next->camera, renderState, next->aspectRatio, &next->perspectiveCorrect);

    next->currentDepth = current->currentDepth - 1;

    next->minX = minX;
    next->minY = minY;
    next->maxX = maxX;
    next->maxY = maxY;

    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, minX, minY, maxX, maxY);
}

void portalInit(struct Portal* portal, enum PortalFlags flags) {
    transformInitIdentity(&portal->transform);
    portal->flags = flags;
}

void portalRender(struct Portal* portal, struct Portal* otherPortal, struct RenderProps* props, SceneRenderCallback sceneRenderer, void* data, struct RenderState* renderState) {
    if (props->currentDepth == 0) {
        Mtx* matrix = renderStateRequestMatrices(renderState, 1);

        transformToMatrixL(&portal->transform, matrix, SCENE_SCALE);
        gSPMatrix(renderState->dl++, matrix, G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL);
        gDPSetPrimColor(renderState->dl++, 0, 0, 255, 128, 0, 255);
        gSPDisplayList(renderState->dl++, portal_outline_portal_outline_mesh);
        gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
        return;
    }
    
    struct ScreenClipper clipper;
    float portalTransform[4][4];

    struct Transform finalTransform;

    finalTransform = portal->transform;

    if (portal->flags & PortalFlagsOddParity) {
        quatMultiply(&portal->transform.rotation, &gVerticalFlip, &finalTransform.rotation);
    }
    
    transformToMatrix(&finalTransform, portalTransform, SCENE_SCALE);

    screenClipperInitWithCamera(&clipper, &props->camera, (float)SCREEN_WD / (float)SCREEN_HT, portalTransform);

    struct Box2D clippingBounds;

    screenClipperBoundingPoints(&clipper, gPortalOutline, sizeof(gPortalOutline) / sizeof(*gPortalOutline), &clippingBounds);

    if (clippingBounds.min.x < clippingBounds.max.x && clippingBounds.min.y < clippingBounds.max.y) {
        int minX = CALC_SCREEN_SPACE(clippingBounds.min.x, SCREEN_WD);
        int maxX = CALC_SCREEN_SPACE(clippingBounds.max.x, SCREEN_WD);
        int minY = CALC_SCREEN_SPACE(-clippingBounds.max.y, SCREEN_HT);
        int maxY = CALC_SCREEN_SPACE(-clippingBounds.min.y, SCREEN_HT);

        struct RenderProps nextProps;
        renderPropsNext(props, &nextProps, &portal->transform, &otherPortal->transform, minX, minY, maxX, maxY, renderState);
        sceneRenderer(data, &nextProps, renderState);

        // revert to previous state
        gSPMatrix(renderState->dl++, osVirtualToPhysical(props->perspectiveMatrix), G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
        gSPPerspNormalize(renderState->dl++, props->perspectiveCorrect);
        gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, props->minX, props->minY, props->maxX, props->maxY);       

        Mtx* matrix = renderStateRequestMatrices(renderState, 1);

        guMtxF2L(portalTransform, matrix);
        gSPMatrix(renderState->dl++, matrix, G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL);
        gSPDisplayList(renderState->dl++, portal_mask_Circle_mesh);
        if (portal->flags & PortalFlagsOddParity) {
            gDPSetPrimColor(renderState->dl++, 0, 0, 0, 128, 255, 255);
        } else {
            gDPSetPrimColor(renderState->dl++, 0, 0, 255, 128, 0, 255);
        }
        gSPDisplayList(renderState->dl++, portal_outline_portal_outline_mesh);
        gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
    }

}