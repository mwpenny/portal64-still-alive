#include "portal.h"

#include "models/models.h"
#include "../graphics/screen_clipper.h"
#include "../graphics/graphics.h"
#include "../defs.h"
#include "dynamic_scene.h"
#include "../physics/collision_scene.h"
#include "../math/mathf.h"
#include "../math/vector2s16.h"
#include "../util/time.h"
#include "../levels/levels.h"
#include "./portal_surface_generator.h"
#include "../controls/controller.h"

#include "../build/assets/models/portal/portal_blue.h"
#include "../build/assets/models/portal/portal_blue_filled.h"
#include "../build/assets/models/portal/portal_blue_face.h"
#include "../build/assets/models/portal/portal_orange.h"
#include "../build/assets/models/portal/portal_orange_face.h"
#include "../build/assets/models/portal/portal_orange_filled.h"


#define CALC_SCREEN_SPACE(clip_space, screen_size) ((clip_space + 1.0f) * ((screen_size) / 2))

#define PORTAL_COVER_HEIGHT 0.708084f
#define PORTAL_COVER_WIDTH  0.84085f

struct Vector3 gPortalOutline[PORTAL_LOOP_SIZE] = {
    {0.0f, 1.0f * SCENE_SCALE * PORTAL_COVER_HEIGHT, 0},
    {0.353553f * SCENE_SCALE * PORTAL_COVER_WIDTH, 0.707107f * SCENE_SCALE * PORTAL_COVER_HEIGHT, 0},
    {0.5f * SCENE_SCALE * PORTAL_COVER_WIDTH, 0.0f, 0},
    {0.353553f * SCENE_SCALE * PORTAL_COVER_WIDTH, -0.707107f * SCENE_SCALE * PORTAL_COVER_HEIGHT, 0},
    {0.0f, -1.0f * SCENE_SCALE * PORTAL_COVER_HEIGHT, 0},
    {-0.353553f * SCENE_SCALE * PORTAL_COVER_WIDTH, -0.707107f * SCENE_SCALE * PORTAL_COVER_HEIGHT, 0},
    {-0.5f * SCENE_SCALE * PORTAL_COVER_WIDTH, 0.0f, 0},
    {-0.353553f * SCENE_SCALE * PORTAL_COVER_WIDTH, 0.707107f * SCENE_SCALE * PORTAL_COVER_HEIGHT, 0},
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

#define PORTAL_CLIPPING_OFFSET  0.1f

#define PORTAL_OPACITY_FADE_TIME    0.6f
#define PORTAL_GROW_TIME            0.3f

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
                props->cullingInfo.clippingPlanes[5].d = (props->cullingInfo.clippingPlanes[5].d + PORTAL_CLIPPING_OFFSET) * SCENE_SCALE;
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

    next->camera.nearPlane = (-vector3Dot(&portalOffset, &cameraForward)) * SCENE_SCALE;

    if (next->camera.nearPlane < current->camera.nearPlane) {
        next->camera.nearPlane = current->camera.nearPlane;

        if (next->camera.nearPlane > next->camera.farPlane) {
            next->camera.nearPlane = next->camera.farPlane;
        }
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

    if (current->clippingPortalIndex == -1) {
        // set the near clipping plane to be the exit portal surface
        quatMultVector(&toPortal->rotation, &gForward, &next->cullingInfo.clippingPlanes[4].normal);
        if (toPortal < fromPortal) {
            vector3Negate(&next->cullingInfo.clippingPlanes[4].normal, &next->cullingInfo.clippingPlanes[4].normal);
        }
        next->cullingInfo.clippingPlanes[4].d = -vector3Dot(&next->cullingInfo.clippingPlanes[4].normal, &toPortal->position) * SCENE_SCALE;
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
    portal->opacity = 1.0f;
    portal->scale = 0.0f;
    portal->portalSurfaceIndex = -1;
}

void portalUpdate(struct Portal* portal, int isOpen) {
    if (isOpen && portal->opacity > 0.0f) {
        portal->opacity -= FIXED_DELTA_TIME * (1.0f / PORTAL_OPACITY_FADE_TIME);

        if (portal->opacity < 0.0f) {
            portal->opacity = 0.0f;
        }
    } else if (!isOpen) {
        portal->opacity = 1.0f;
    }

    if (controllerGetButton(1, B_BUTTON)) {
        portal->opacity = 0.5f;
    }

    if (portal->scale < 1.0f) {
        portal->scale += FIXED_DELTA_TIME * (1.0f / PORTAL_GROW_TIME);

        if (portal->scale > 1.0f) {
            portal->scale = 1.0f;
        }

        portal->flags |= PortalFlagsNeedsNewHole;
    }
}

void portalRenderScreenCover(struct Vector2s16* points, int pointCount, struct RenderProps* props, struct RenderState* renderState) {
    if (pointCount == 0) {
        return;
    }

    gDPPipeSync(renderState->dl++);
    gDPSetDepthSource(renderState->dl++, G_ZS_PRIM);
    gDPSetPrimDepth(renderState->dl++, 0, 0);
    gDPSetRenderMode(renderState->dl++, CLR_ON_CVG | FORCE_BL | IM_RD | Z_CMP | Z_UPD | CVG_DST_WRAP | ZMODE_OPA | GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA), CLR_ON_CVG | FORCE_BL | IM_RD | Z_CMP | Z_UPD | CVG_DST_WRAP | ZMODE_OPA | GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA));
    gDPSetCombineLERP(renderState->dl++, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE);

    if (controllerGetButton(1, D_CBUTTONS)) {
        gDPSetPrimColor(renderState->dl++, 255, 255, 0, 0, 0, 128);
    }

    Mtx* idenity = renderStateRequestMatrices(renderState, 1);
    guMtxIdent(idenity);

    gSPMatrix(renderState->dl++, idenity, G_MTX_LOAD | G_MTX_MODELVIEW | G_MTX_PUSH);

    Mtx* ortho = renderStateRequestMatrices(renderState, 1);
    guOrtho(ortho, 0, SCREEN_WD << 2, 0, SCREEN_HT << 2, -10, 10, 1);

    gSPMatrix(renderState->dl++, ortho, G_MTX_LOAD | G_MTX_PROJECTION | G_MTX_NOPUSH);

    Vtx* vertices = renderStateRequestVertices(renderState, pointCount);

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
    gSPMatrix(renderState->dl++, props->perspectiveMatrix, G_MTX_LOAD | G_MTX_PROJECTION | G_MTX_NOPUSH);
    gDPSetDepthSource(renderState->dl++, G_ZS_PIXEL);
    gDPSetRenderMode(renderState->dl++, G_RM_ZB_OPA_SURF, G_RM_ZB_OPA_SURF2);
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
    
    vector3Scale(&gOneVec, &finalTransform.scale, portal->scale);

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

    screenClipperInitWithCamera(&clipper, &props->camera, (float)SCREEN_WD / (float)SCREEN_HT, portalTransform);
    struct Box2D clippingBounds;
    screenClipperBoundingPoints(&clipper, gPortalOutline, sizeof(gPortalOutline) / sizeof(*gPortalOutline), &clippingBounds);

    if (props->clippingPortalIndex == portalIndex) {
        nextProps.minX = 0;
        nextProps.maxX = SCREEN_WD;
        nextProps.minY = 0;
        nextProps.maxY = SCREEN_HT;
    } else {
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

        gDPSetEnvColor(renderState->dl++, 255, 255, 255, portal->opacity < 0.0f ? 0 : (portal->opacity > 1.0f ? 255 : (u8)(portal->opacity * 255.0f)));
        
        if (portal->flags & PortalFlagsOddParity) {
            if (!controllerGetButton(1, L_CBUTTONS)) {
                gSPDisplayList(renderState->dl++, portal_portal_blue_face_model_gfx);
                if (!controllerGetButton(1, U_CBUTTONS)) {
                    portalRenderScreenCover(clipper.nearPolygon, clipper.nearPolygonCount, props, renderState);
                }
            }
            gDPPipeSync(renderState->dl++);

            gSPDisplayList(renderState->dl++, portal_portal_blue_model_gfx);
        } else {
            if (!controllerGetButton(1, L_CBUTTONS)) {
                gSPDisplayList(renderState->dl++, portal_portal_orange_face_model_gfx);
                if (!controllerGetButton(1, U_CBUTTONS)) {
                    portalRenderScreenCover(clipper.nearPolygon, clipper.nearPolygonCount, props, renderState);
                }
            }
            gDPPipeSync(renderState->dl++);

            gSPDisplayList(renderState->dl++, portal_portal_orange_model_gfx);
        }
        
        gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
    }

}

int portalAttachToSurface(struct Portal* portal, struct PortalSurface* surface, int surfaceIndex, struct Transform* portalAt) {
    // determine if portal is on surface
    if (!portalSurfaceIsInside(surface, portalAt)) {
        return 0;
    }
    // find all portal edge intersections
    struct Vector2s16 correctPosition;
    struct Vector2s16 portalOutline[PORTAL_LOOP_SIZE];
    if (!portalSurfaceAdjustPosition(surface, portalAt, &correctPosition, portalOutline)) {
        return 0;
    }

    for (int i = 0; i < PORTAL_LOOP_SIZE; ++i) {
        vector2s16Sub(&portalOutline[i], &correctPosition, &portal->originCentertedLoop[i]);
    }

    portal->flags |= PortalFlagsNeedsNewHole;
    portal->fullSizeLoopCenter = correctPosition;
    portal->portalSurfaceIndex = surfaceIndex;
    
    portalSurfaceInverse(surface, &correctPosition, &portalAt->position);

    return 1;
}

int portalSurfaceCutNewHole(struct Portal* portal, int portalIndex) {
    portal->flags &= ~PortalFlagsNeedsNewHole;

    if (portal->portalSurfaceIndex == -1) {
        return 1;
    }

    struct Vector2s16 scaledLoop[PORTAL_LOOP_SIZE];

    int fixedPointScale = (int)(0x10000 * portal->scale);

    for (int i = 0; i < PORTAL_LOOP_SIZE; ++i) {
        scaledLoop[i].x = ((portal->originCentertedLoop[i].x * fixedPointScale) >> 16) + portal->fullSizeLoopCenter.x;
        scaledLoop[i].y = ((portal->originCentertedLoop[i].y * fixedPointScale) >> 16) + portal->fullSizeLoopCenter.y;
    }

    struct PortalSurface* currentSurface = &gCurrentLevel->portalSurfaces[portal->portalSurfaceIndex];

    struct PortalSurface newSurface;

    if (!portalSurfacePokeHole(currentSurface, scaledLoop, &newSurface)) {
        return 0;
    }

    portalSurfaceReplace(portal->portalSurfaceIndex, portal->roomIndex, portalIndex, &newSurface);

    return 1;
}

void portalCheckForHoles(struct Portal* portals) {
    if (controllerGetButtonUp(1, R_CBUTTONS)) {
        portals[0].flags |= PortalFlagsNeedsNewHole;
        portals[1].flags |= PortalFlagsNeedsNewHole;
    }

    if ((portals[1].flags & PortalFlagsNeedsNewHole) != 0 || (
        portalSurfaceAreBothOnSameSurface() && (portals[0].flags & PortalFlagsNeedsNewHole) != 0
    )) {
        portalSurfaceRevert(1);
        portals[1].flags |= PortalFlagsNeedsNewHole;
    }

    if ((portals[0].flags & PortalFlagsNeedsNewHole) != 0) {
        portalSurfaceRevert(0);
    }

    if (controllerGetButton(1, R_CBUTTONS)) {
        portalSurfaceRevert(1);
        portalSurfaceRevert(0);
        return;
    }

    if ((portals[0].flags & PortalFlagsNeedsNewHole) != 0) {
        portalSurfaceCutNewHole(&portals[0], 0);
    }

    if ((portals[1].flags & PortalFlagsNeedsNewHole) != 0) {
        portalSurfaceCutNewHole(&portals[1], 1);
    }
    
}