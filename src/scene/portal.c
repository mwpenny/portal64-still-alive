#include "portal.h"

#include "models/models.h"
#include "../graphics/screen_clipper.h"
#include "../graphics/graphics.h"
#include "../defs.h"
#include "dynamic_scene.h"
#include "../physics/collision_scene.h"
#include "../math/mathf.h"
#include "../math/vector2s16.h"

#include "../build/assets/models/portal/portal_blue.h"
#include "../build/assets/models/portal/portal_orange.h"
#include "../build/assets/models/portal/portal_blue_filled.h"
#include "../build/assets/models/portal/portal_orange_filled.h"

#include "../build/assets/models/portal/portal_face.h"

#define CALC_SCREEN_SPACE(clip_space, screen_size) ((clip_space + 1.0f) * ((screen_size) / 2))

#define PORTAL_SCALE_Y  0.8f
#define PORTAL_SCALE_X  0.95f

#define PORTAL_FACE_OFFSET  (-0.014106 * SCENE_SCALE)

struct Vector3 gPortalOutline[PORTAL_LOOP_SIZE] = {
    {-0.353553f * SCENE_SCALE * PORTAL_SCALE_X, 0.707107f * SCENE_SCALE * PORTAL_SCALE_Y, PORTAL_FACE_OFFSET},
    {-0.5f * SCENE_SCALE* PORTAL_SCALE_X, 0.0f, PORTAL_FACE_OFFSET},
    {-0.353553f * SCENE_SCALE * PORTAL_SCALE_X, -0.707107f * SCENE_SCALE * PORTAL_SCALE_Y, PORTAL_FACE_OFFSET},
    {0.0f, -1.0f * SCENE_SCALE * PORTAL_SCALE_Y, PORTAL_FACE_OFFSET},
    {0.353553f * SCENE_SCALE * PORTAL_SCALE_X, -0.707107f * SCENE_SCALE * PORTAL_SCALE_Y, PORTAL_FACE_OFFSET},
    {0.5f * SCENE_SCALE * PORTAL_SCALE_X, 0.0f, PORTAL_FACE_OFFSET},
    {0.353553f * SCENE_SCALE * PORTAL_SCALE_X, 0.707107f * SCENE_SCALE * PORTAL_SCALE_Y, PORTAL_FACE_OFFSET},
    {0.0f, 1.0f * SCENE_SCALE * PORTAL_SCALE_Y, PORTAL_FACE_OFFSET},
};

struct Vector3 gPortalOutlineWorld[PORTAL_LOOP_SIZE] = {
    {-0.353553f * PORTAL_SCALE_X, 0.707107f * PORTAL_SCALE_Y, 0.0f},
    {-0.5f * PORTAL_SCALE_X, 0.0f, 0.0f},
    {-0.353553f * PORTAL_SCALE_X, -0.707107f * PORTAL_SCALE_Y, 0.0f},
    {0.0f, -1.0f * PORTAL_SCALE_Y, 0.0f},
    {0.353553f * PORTAL_SCALE_X, -0.707107f * PORTAL_SCALE_Y, 0.0f},
    {0.5f * PORTAL_SCALE_X, 0.0f, 0.0f},
    {0.353553f * PORTAL_SCALE_X, 0.707107f * PORTAL_SCALE_Y, 0.0f},
    {0.0f, 1.0f * PORTAL_SCALE_Y, 0.0f},
};

#define SHOW_EXTERNAL_VIEW  0

#if SHOW_EXTERNAL_VIEW

struct Vector3 externalCameraPos = {16.0f, 8.0f, -16.0f};
struct Vector3 externalLook = {4.0f, 2.0f, -4.0f};

#endif

struct Quaternion gVerticalFlip = {0.0f, 1.0f, 0.0f, 0.0f};

#define STARTING_RENDER_DEPTH       2

#define PORTAL_CLIPPING_PLANE_BIAS  (SCENE_SCALE * 0.25f)

#define CAMERA_CLIPPING_RADIUS  0.2f

void renderPropsInit(struct RenderProps* props, struct Camera* camera, float aspectRatio, struct RenderState* renderState, u16 roomIndex) {
    props->camera = *camera;
    props->aspectRatio = aspectRatio;
    props->perspectiveMatrix = cameraSetupMatrices(camera, renderState, aspectRatio, &props->perspectiveCorrect, &fullscreenViewport, &props->cullingInfo);
    props->viewport = &fullscreenViewport;
    props->currentDepth = STARTING_RENDER_DEPTH;
    props->fromPortalIndex = NO_PORTAL;
    props->fromRoom = roomIndex;

#if SHOW_EXTERNAL_VIEW
    struct Camera externalCamera = *camera;
    externalCamera.transform.position = externalCameraPos;
    struct Vector3 offset;
    vector3Sub(&externalLook, &externalCameraPos, &offset);
    quatLook(&offset, &gUp, &externalCamera.transform.rotation);
    props->perspectiveMatrix = cameraSetupMatrices(&externalCamera, renderState, aspectRatio, &props->perspectiveCorrect, &fullscreenViewport, NULL);
#endif

    props->clippingPortalIndex = -1;

    if (collisionSceneIsPortalOpen()) {
        for (int i = 0; i < 2; ++i) {
            struct Vector3 portalOffset;

            vector3Sub(&camera->transform.position, &gCollisionScene.portalTransforms[i]->position, &portalOffset);

            struct Vector3 portalNormal;
            quatMultVector(&gCollisionScene.portalTransforms[i]->rotation, &gForward, &portalNormal);
            struct Vector3 projectedPoint;

            if (i == 0) {
                vector3Negate(&portalNormal, &portalNormal);
            }

            float clippingDistnace = vector3Dot(&portalNormal, &portalOffset);

            if (fabsf(clippingDistnace) > CAMERA_CLIPPING_RADIUS) {
                continue;
            }

            vector3AddScaled(&camera->transform.position, &portalNormal, -clippingDistnace, &projectedPoint);

            if (collisionSceneIsTouchingSinglePortal(&projectedPoint, &portalNormal, gCollisionScene.portalTransforms[i], i)) {
                planeInitWithNormalAndPoint(&props->cullingInfo.clippingPlanes[5], &portalNormal, &gCollisionScene.portalTransforms[i]->position);
                props->cullingInfo.clippingPlanes[5].d *= SCENE_SCALE;
                ++props->cullingInfo.usedClippingPlaneCount;
                props->clippingPortalIndex = i;
                break;
            }
        }
    }

    props->minX = 0;
    props->minY = 0;
    props->maxX = SCREEN_WD;
    props->maxY = SCREEN_HT;
}

#define MIN_VP_WIDTH 64

void renderPropscheckViewportSize(int* min, int* max, int screenSize) {
    if (*max < MIN_VP_WIDTH) {
        *max = MIN_VP_WIDTH;
    }

    if (*min > screenSize - MIN_VP_WIDTH) {
        *min = screenSize - MIN_VP_WIDTH;
    }

    int widthGrowBy = MIN_VP_WIDTH - (*max - *min);

    if (widthGrowBy > 0) {
        *min -= widthGrowBy >> 1;
        *max += (widthGrowBy + 1) >> 1;
    }
}

int renderPropsZDistance(int currentDepth) {
    if (currentDepth >= STARTING_RENDER_DEPTH) {
        return 0;
    } else if (currentDepth < 0) {
        return G_MAXZ;
    } else {
        return G_MAXZ - (G_MAXZ >> (STARTING_RENDER_DEPTH - currentDepth));
    }
}

Vp* renderPropsBuildViewport(struct RenderProps* props, struct RenderState* renderState) {
    int minX = props->minX;
    int maxX = props->maxX;
    int minY = props->minY;
    int maxY = props->maxY;

    int minZ = renderPropsZDistance(props->currentDepth);
    int maxZ = renderPropsZDistance(props->currentDepth - 1);

    renderPropscheckViewportSize(&minX, &maxX, SCREEN_WD);
    renderPropscheckViewportSize(&minY, &maxY, SCREEN_HT);

    Vp* viewport = renderStateRequestViewport(renderState);

    viewport->vp.vscale[0] = (maxX - minX) << 1;
    viewport->vp.vscale[1] = (maxY - minY) << 1;
    viewport->vp.vscale[2] = (maxZ - minZ) >> 1;
    viewport->vp.vscale[3] = 0;

    viewport->vp.vtrans[0] = (maxX + minX) << 1;
    viewport->vp.vtrans[1] = (maxY + minY) << 1;
    viewport->vp.vtrans[2] = (maxZ + minZ) >> 1;
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

    struct Vector3 portalOffset;
    vector3Sub(&toPortal->position, &next->camera.transform.position, &portalOffset);

    struct Vector3 cameraForward;
    quatMultVector(&next->camera.transform.rotation, &gForward, &cameraForward);

    next->camera.nearPlane = (-vector3Dot(&portalOffset, &cameraForward) - PORTAL_SCALE_Y) * SCENE_SCALE;

    if (next->camera.nearPlane < current->camera.nearPlane) {
        next->camera.nearPlane = current->camera.nearPlane;
    }

    // render any objects halfway through portals
    cameraSetupMatrices(&next->camera, renderState, next->aspectRatio, &next->perspectiveCorrect, current->viewport, NULL);
    dynamicSceneRenderTouchingPortal(&next->camera.transform, &current->cullingInfo, renderState);

    next->currentDepth = current->currentDepth - 1;
    Vp* viewport = renderPropsBuildViewport(next, renderState);

    next->viewport = viewport;

    next->perspectiveMatrix = cameraSetupMatrices(&next->camera, renderState, next->aspectRatio, &next->perspectiveCorrect, viewport, &next->cullingInfo);

#if SHOW_EXTERNAL_VIEW
    struct Transform externalTransform;
    externalTransform.position = externalCameraPos;
    externalTransform.scale = gOneVec;
    struct Vector3 offset;
    vector3Sub(&externalLook, &externalCameraPos, &offset);
    quatLook(&offset, &gUp, &externalTransform.rotation);
    struct Camera externalCamera = current->camera;
    transformConcat(&portalCombined, &externalTransform, &externalCamera.transform);
    next->perspectiveMatrix = cameraSetupMatrices(&externalCamera, renderState, (float)SCREEN_WD / (float)SCREEN_HT, &next->perspectiveCorrect, &fullscreenViewport, NULL);
#endif

    if (current->clippingPortalIndex != -1) {
        // set the near clipping plane to be the exit portal surface
        quatMultVector(&toPortal->rotation, &gForward, &next->cullingInfo.clippingPlanes[4].normal);
        if (toPortal < fromPortal) {
            vector3Negate(&next->cullingInfo.clippingPlanes[4].normal, &next->cullingInfo.clippingPlanes[4].normal);
        }
        next->cullingInfo.clippingPlanes[4].d = -vector3Dot(&next->cullingInfo.clippingPlanes[4].normal, &toPortal->position) * SCENE_SCALE - PORTAL_CLIPPING_PLANE_BIAS;
    }
    next->clippingPortalIndex = -1;

    next->fromPortalIndex = toPortal < fromPortal ? 0 : 1;
    // Gross
    next->fromRoom = gCollisionScene.portalRooms[toPortal == gCollisionScene.portalTransforms[0] ? 0 : 1];

#if !SHOW_EXTERNAL_VIEW
    gSPViewport(renderState->dl++, viewport);
    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, next->minX, next->minY, next->maxX, next->maxY);
#endif
}

void portalInit(struct Portal* portal, enum PortalFlags flags) {
    transformInitIdentity(&portal->transform);
    portal->flags = flags;
}

void portalRender(struct Portal* portal, struct Portal* otherPortal, struct RenderProps* props, SceneRenderCallback sceneRenderer, void* data, struct RenderState* renderState) {
    struct Vector3 forward = gForward;
    if (!(portal->flags & PortalFlagsOddParity)) {
        forward.z = -1.0f;
    }

    struct Vector3 worldForward;
    quatMultVector(&portal->transform.rotation, &forward, &worldForward);

    struct Vector3 offsetFromCamera;
    vector3Sub(&props->camera.transform.position, &portal->transform.position, &offsetFromCamera);

    // don't render the portal if it is facing the wrong way
    if (vector3Dot(&worldForward, &offsetFromCamera) < 0.0f) {
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

    if (props->currentDepth == 0 || !otherPortal) {
        Mtx* matrix = renderStateRequestMatrices(renderState, 1);

        guMtxF2L(portalTransform, matrix);
        gSPMatrix(renderState->dl++, matrix, G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL);
        if (portal->flags & PortalFlagsOddParity) {
            gSPDisplayList(renderState->dl++, portal_portal_blue_filled_model_gfx);
        } else {
            gSPDisplayList(renderState->dl++, portal_portal_orange_filled_model_gfx);
        }
        gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
        return;
    }

    struct RenderProps nextProps;
    int portalIndex = portal < otherPortal ? 0 : 1;

    if (props->clippingPortalIndex == portalIndex) {
        nextProps.minX = 0;
        nextProps.maxX = SCREEN_WD;
        nextProps.minY = 0;
        nextProps.maxY = SCREEN_HT;
    } else {

        screenClipperInitWithCamera(&clipper, &props->camera, (float)SCREEN_WD / (float)SCREEN_HT, portalTransform);

        struct Box2D clippingBounds;

        screenClipperBoundingPoints(&clipper, gPortalOutline, sizeof(gPortalOutline) / sizeof(*gPortalOutline), &clippingBounds);

        nextProps.minX = CALC_SCREEN_SPACE(clippingBounds.min.x, SCREEN_WD);
        nextProps.maxX = CALC_SCREEN_SPACE(clippingBounds.max.x, SCREEN_WD);
        nextProps.minY = CALC_SCREEN_SPACE(-clippingBounds.max.y, SCREEN_HT);
        nextProps.maxY = CALC_SCREEN_SPACE(-clippingBounds.min.y, SCREEN_HT);

        nextProps.minX = MAX(nextProps.minX, props->minX);
        nextProps.maxX = MIN(nextProps.maxX, props->maxX);
        nextProps.minY = MAX(nextProps.minY, props->minY);
        nextProps.maxY = MIN(nextProps.maxY, props->maxY);
    }

    if (nextProps.minX < nextProps.maxX && nextProps.minY < nextProps.maxY) {
        renderPropsNext(props, &nextProps, &portal->transform, &otherPortal->transform, renderState);
        sceneRenderer(data, &nextProps, renderState);

        // revert to previous state
        gSPMatrix(renderState->dl++, osVirtualToPhysical(props->perspectiveMatrix), G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
        gSPPerspNormalize(renderState->dl++, props->perspectiveCorrect);
        gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, props->minX, props->minY, props->maxX, props->maxY);       
        gSPViewport(renderState->dl++, props->viewport);

        // render the front portal cover
        Mtx* matrix = renderStateRequestMatrices(renderState, 1);

        guMtxF2L(portalTransform, matrix);
        gSPMatrix(renderState->dl++, matrix, G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL);
        
        gSPDisplayList(renderState->dl++, portal_portal_face_model_gfx);
        gDPPipeSync(renderState->dl++);
        if (portal->flags & PortalFlagsOddParity) {
            gSPDisplayList(renderState->dl++, portal_portal_blue_model_gfx);
        } else {
            gSPDisplayList(renderState->dl++, portal_portal_orange_model_gfx);
        }
        
        gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
    }

}