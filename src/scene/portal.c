#include "portal.h"

#include "models/models.h"
#include "../graphics/screen_clipper.h"
#include "../graphics/graphics.h"
#include "../defs.h"
#include "dynamic_scene.h"

#define CALC_SCREEN_SPACE(clip_space, screen_size) ((clip_space + 1.0f) * ((screen_size) / 2))

struct Vector3 gPortalOutline[PORTAL_LOOP_SIZE] = {
    {0.0f, 1.0f * SCENE_SCALE, 0.0f},
    {0.353553f * SCENE_SCALE, 0.707107f * SCENE_SCALE, 0.0f},
    {0.5f * SCENE_SCALE, 0.0f, 0.0f},
    {0.353553f * SCENE_SCALE, -0.707107f * SCENE_SCALE, 0.0f},
    {0.0f, -1.0f * SCENE_SCALE, 0.0f},
    {-0.353553f * SCENE_SCALE, -0.707107f * SCENE_SCALE, 0.0f},
    {-0.5f * SCENE_SCALE, 0.0f, 0.0f},
    {-0.353553f * SCENE_SCALE, 0.707107f * SCENE_SCALE, 0.0f},
};

struct Vector3 gPortalOutlineUnscaled[PORTAL_LOOP_SIZE] = {
    {0.0f, 1.0f, 0.0f},
    {0.353553f, 0.707107f, 0.0f},
    {0.5f, 0.0f, 0.0f},
    {0.353553f, -0.707107f, 0.0f},
    {0.0f, -1.0f, 0.0f},
    {-0.353553f, -0.707107f, 0.0f},
    {-0.5f, 0.0f, 0.0f},
    {-0.353553f, 0.707107f, 0.0f},
};

struct Quaternion gVerticalFlip = {0.0f, 1.0f, 0.0f, 0.0f};

void renderPropsInit(struct RenderProps* props, struct Camera* camera, float aspectRatio, struct RenderState* renderState) {
    props->camera = *camera;
    props->aspectRatio = aspectRatio;
    props->perspectiveMatrix = cameraSetupMatrices(camera, renderState, aspectRatio, &props->perspectiveCorrect, &fullscreenViewport);
    props->viewport = &fullscreenViewport;
    props->currentDepth = STARTING_RENDER_DEPTH;

    props->minX = 0;
    props->minY = 0;
    props->maxX = SCREEN_WD;
    props->maxY = SCREEN_HT;
}

#define MIN_VP_WIDTH 40

Vp* renderPropsBuildViewport(struct RenderProps* props, struct RenderState* renderState) {
    int minX = props->minX;
    int maxX = props->maxX;

    if (maxX < MIN_VP_WIDTH) {
        maxX = MIN_VP_WIDTH;
    }

    if (minX > SCREEN_WD - MIN_VP_WIDTH) {
        minX = SCREEN_WD - MIN_VP_WIDTH;
    }

    Vp* viewport = renderStateRequestViewport(renderState);

    viewport->vp.vscale[0] = (maxX - minX) << 1;
    viewport->vp.vscale[1] = (props->maxY - props->minY) << 1;
    viewport->vp.vscale[2] = G_MAXZ/2;
    viewport->vp.vscale[3] = 0;

    viewport->vp.vtrans[0] = (maxX + minX) << 1;
    viewport->vp.vtrans[1] = (props->maxY + props->minY) << 1;
    viewport->vp.vtrans[2] = G_MAXZ/2;
    viewport->vp.vtrans[3] = 0;

    return viewport;
}

void renderPropsNext(struct RenderProps* current, struct RenderProps* next, struct Transform* fromPortal, struct Transform* toPortal, struct RenderState* renderState) {
    struct Transform otherInverse;
    transformInvert(fromPortal, &otherInverse);
    struct Transform portalCombined;
    transformConcat(toPortal, &otherInverse, &portalCombined);

    next->camera = current->camera;
    next->aspectRatio = current->aspectRatio;
    transformConcat(&portalCombined, &current->camera.transform, &next->camera.transform);

    // render any objects halfway through portals
    cameraSetupMatrices(&next->camera, renderState, next->aspectRatio, &next->perspectiveCorrect, current->viewport);
    dynamicSceneRender(renderState, 1);

    Vp* viewport = renderPropsBuildViewport(next, renderState);

    next->viewport = viewport;

    next->perspectiveMatrix = cameraSetupMatrices(&next->camera, renderState, next->aspectRatio, &next->perspectiveCorrect, viewport);

    next->currentDepth = current->currentDepth - 1;

    gSPViewport(renderState->dl++, viewport);
    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, next->minX, next->minY, next->maxX, next->maxY);
}

void portalInit(struct Portal* portal, enum PortalFlags flags) {
    transformInitIdentity(&portal->transform);
    portal->flags = flags;
}

void portalRender(struct Portal* portal, struct Portal* otherPortal, struct RenderProps* props, SceneRenderCallback sceneRenderer, void* data, struct RenderState* renderState) {
    struct ScreenClipper clipper;
    float portalTransform[4][4];

    struct Transform finalTransform;

    finalTransform = portal->transform;

    if (portal->flags & PortalFlagsOddParity) {
        quatMultiply(&portal->transform.rotation, &gVerticalFlip, &finalTransform.rotation);
    }
    
    transformToMatrix(&finalTransform, portalTransform, SCENE_SCALE);

    if (props->currentDepth == 0) {
        Mtx* matrix = renderStateRequestMatrices(renderState, 1);

        guMtxF2L(portalTransform, matrix);
        gSPMatrix(renderState->dl++, matrix, G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL);
        gDPPipeSync(renderState->dl++);
        if (portal->flags & PortalFlagsOddParity) {
            gDPSetPrimColor(renderState->dl++, 0, 0, 0, 128, 255, 255);
        } else {
            gDPSetPrimColor(renderState->dl++, 0, 0, 255, 128, 0, 255);
        }
        gSPDisplayList(renderState->dl++, portal_outline_portal_outline_mesh);
        gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
        return;
    }

    screenClipperInitWithCamera(&clipper, &props->camera, (float)SCREEN_WD / (float)SCREEN_HT, portalTransform);

    struct Box2D clippingBounds;

    screenClipperBoundingPoints(&clipper, gPortalOutline, sizeof(gPortalOutline) / sizeof(*gPortalOutline), &clippingBounds);

    if (clippingBounds.min.x < clippingBounds.max.x && clippingBounds.min.y < clippingBounds.max.y) {
        struct RenderProps nextProps;

        nextProps.minX = CALC_SCREEN_SPACE(clippingBounds.min.x, SCREEN_WD);
        nextProps.maxX = CALC_SCREEN_SPACE(clippingBounds.max.x, SCREEN_WD);
        nextProps.minY = CALC_SCREEN_SPACE(-clippingBounds.max.y, SCREEN_HT);
        nextProps.maxY = CALC_SCREEN_SPACE(-clippingBounds.min.y, SCREEN_HT);

        nextProps.minX = MAX(nextProps.minX, props->minX);
        nextProps.maxX = MIN(nextProps.maxX, props->maxX);
        nextProps.minY = MAX(nextProps.minY, props->minY);
        nextProps.maxY = MIN(nextProps.maxY, props->maxY);

        renderPropsNext(props, &nextProps, &portal->transform, &otherPortal->transform, renderState);
        sceneRenderer(data, &nextProps, renderState);

        // revert to previous state
        gSPMatrix(renderState->dl++, osVirtualToPhysical(props->perspectiveMatrix), G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
        gSPPerspNormalize(renderState->dl++, props->perspectiveCorrect);
        gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, props->minX, props->minY, props->maxX, props->maxY);       
        gSPViewport(renderState->dl++, props->viewport);

        Mtx* matrix = renderStateRequestMatrices(renderState, 1);

        guMtxF2L(portalTransform, matrix);
        gSPMatrix(renderState->dl++, matrix, G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL);
        gSPDisplayList(renderState->dl++, portal_mask_Circle_mesh);
        gDPPipeSync(renderState->dl++);
        if (portal->flags & PortalFlagsOddParity) {
            gDPSetPrimColor(renderState->dl++, 0, 0, 0, 128, 255, 255);
        } else {
            gDPSetPrimColor(renderState->dl++, 0, 0, 255, 128, 0, 255);
        }
        gSPDisplayList(renderState->dl++, portal_outline_portal_outline_mesh);
        gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
    }

}