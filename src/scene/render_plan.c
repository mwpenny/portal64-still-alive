
#include "render_plan.h"

#include "../graphics/screen_clipper.h"
#include "../physics/collision_scene.h"
#include "../levels/static_render.h"

#include "../build/assets/models/portal/portal_blue.h"
#include "../build/assets/models/portal/portal_blue_filled.h"
#include "../build/assets/models/portal/portal_blue_face.h"
#include "../build/assets/models/portal/portal_orange.h"
#include "../build/assets/models/portal/portal_orange_face.h"
#include "../build/assets/models/portal/portal_orange_filled.h"

Vp* renderPropsBuildViewport(struct RenderProps* props, struct RenderState* renderState);
void renderPlanFinishView(struct RenderPlan* renderPlan, struct Scene* scene, struct RenderProps* properties, struct RenderState* renderState);
void portalRenderScreenCover(struct Vector2s16* points, int pointCount, struct RenderProps* props, struct RenderState* renderState);
void portalRenderCover(struct Portal* portal, float portalTransform[4][4], struct RenderState* renderState);
extern struct Quaternion gVerticalFlip;
extern struct Vector3 gPortalOutline[PORTAL_LOOP_SIZE];

#define CALC_SCREEN_SPACE(clip_space, screen_size) ((clip_space + 1.0f) * ((screen_size) / 2))

int renderPlanPortal(struct RenderPlan* renderPlan, struct Scene* scene, struct RenderProps* current, int portalIndex, struct RenderState* renderState) {
    int exitPortalIndex = 1 - portalIndex;
    struct Portal* portal = &scene->portals[portalIndex];

    struct Vector3 forward = gForward;
    if (!(portal->flags & PortalFlagsOddParity)) {
        forward.z = -1.0f;
    }

    struct Vector3 worldForward;
    quatMultVector(&portal->transform.rotation, &forward, &worldForward);

    struct Vector3 offsetFromCamera;
    vector3Sub(&current->camera.transform.position, &portal->transform.position, &offsetFromCamera);

    // don't render the portal if it is facing the wrong way
    if (vector3Dot(&worldForward, &offsetFromCamera) < 0.0f) {
        return 0;
    }
    
    int flags = PORTAL_RENDER_TYPE_VISIBLE(portalIndex);

    float portalTransform[4][4];

    portalDetermineTransform(portal, portalTransform);

    if (current->currentDepth == 0 || !collisionSceneIsPortalOpen() || renderPlan->stageCount >= MAX_PORTAL_STEPS) {
        return flags; 
    }

    struct RenderProps* next = &renderPlan->stageProps[renderPlan->stageCount];

    screenClipperInitWithCamera(&next->clipper, &current->camera, (float)SCREEN_WD / (float)SCREEN_HT, portalTransform);
    struct Box2D clippingBounds;
    screenClipperBoundingPoints(&next->clipper, gPortalOutline, sizeof(gPortalOutline) / sizeof(*gPortalOutline), &clippingBounds);

    if (current->clippingPortalIndex == portalIndex) {
        next->minX = 0;
        next->maxX = SCREEN_WD;
        next->minY = 0;
        next->maxY = SCREEN_HT;
    } else {
        next->minX = CALC_SCREEN_SPACE(clippingBounds.min.x, SCREEN_WD);
        next->maxX = CALC_SCREEN_SPACE(clippingBounds.max.x, SCREEN_WD);
        next->minY = CALC_SCREEN_SPACE(-clippingBounds.max.y, SCREEN_HT);
        next->maxY = CALC_SCREEN_SPACE(-clippingBounds.min.y, SCREEN_HT);

        next->minX = MAX(next->minX, current->minX);
        next->maxX = MIN(next->maxX, current->maxX);
        next->minY = MAX(next->minY, current->minY);
        next->maxY = MIN(next->maxY, current->maxY);
    }

    if (next->minX >= next->maxX || next->minY >= next->maxY) {
        return 0;
    }

    struct Transform* fromPortal = &scene->portals[portalIndex].transform;
    struct Transform* exitPortal = &scene->portals[exitPortalIndex].transform;

    struct Transform otherInverse;
    transformInvert(fromPortal, &otherInverse);
    struct Transform portalCombined;
    transformConcat(exitPortal, &otherInverse, &portalCombined);

    next->camera = current->camera;
    next->aspectRatio = current->aspectRatio;
    transformConcat(&portalCombined, &current->camera.transform, &next->camera.transform);

    struct Vector3 portalOffset;
    vector3Sub(&exitPortal->position, &next->camera.transform.position, &portalOffset);

    struct Vector3 cameraForward;
    quatMultVector(&next->camera.transform.rotation, &gForward, &cameraForward);

    next->camera.nearPlane = (-vector3Dot(&portalOffset, &cameraForward)) * SCENE_SCALE;

    if (next->camera.nearPlane < current->camera.nearPlane) {
        next->camera.nearPlane = current->camera.nearPlane;

        if (next->camera.nearPlane > next->camera.farPlane) {
            next->camera.nearPlane = next->camera.farPlane;
        }
    }

    next->currentDepth = current->currentDepth - 1;
    next->viewport = renderPropsBuildViewport(next, renderState);

    if (!next->viewport) {
        return flags;
    }

    if (!cameraSetupMatrices(&next->camera, renderState, next->aspectRatio, next->viewport, &next->cameraMatrixInfo)) {
        return flags;
    }

    if (current->clippingPortalIndex == -1) {
        // set the near clipping plane to be the exit portal surface
        quatMultVector(&exitPortal->rotation, &gForward, &next->cameraMatrixInfo.cullingInformation.clippingPlanes[4].normal);
        if (exitPortal < fromPortal) {
            vector3Negate(&next->cameraMatrixInfo.cullingInformation.clippingPlanes[4].normal, &next->cameraMatrixInfo.cullingInformation.clippingPlanes[4].normal);
        }
        next->cameraMatrixInfo.cullingInformation.clippingPlanes[4].d = -vector3Dot(&next->cameraMatrixInfo.cullingInformation.clippingPlanes[4].normal, &exitPortal->position) * SCENE_SCALE;
    }
    next->clippingPortalIndex = -1;

    next->exitPortalIndex = exitPortalIndex;
    next->fromRoom = gCollisionScene.portalRooms[next->exitPortalIndex];

    ++renderPlan->stageCount;

    next->previousProperties = current;

    next->portalRenderType = 0;

    renderPlanFinishView(renderPlan, scene, next, renderState);

    return flags | PORTAL_RENDER_TYPE_ENABLED(portalIndex);
}

void renderPlanFinishView(struct RenderPlan* renderPlan, struct Scene* scene, struct RenderProps* properties, struct RenderState* renderState) {
    
    staticRenderDetermineVisibleRooms(&properties->cameraMatrixInfo.cullingInformation, properties->fromRoom, &properties->visiblerooms);

    int furtherPortal = vector3DistSqrd(&properties->camera.transform.position, &scene->portals[0].transform.position) > vector3DistSqrd(&properties->camera.transform.position, &scene->portals[1].transform.position) ? 0 : 1;
    int otherPortal = 1 - furtherPortal;
    
    if (!furtherPortal) {
        properties->portalRenderType |= PORTAL_RENDER_TYPE_SECOND_CLOSER;
    }

    for (int i = 0; i < 2; ++i) {
        if (gCollisionScene.portalTransforms[furtherPortal] && 
            properties->exitPortalIndex != furtherPortal && 
            staticRenderIsRoomVisible(properties->visiblerooms, gCollisionScene.portalRooms[furtherPortal])) {
            properties->portalRenderType |= renderPlanPortal(
                renderPlan,
                scene,
                properties,
                furtherPortal,
                renderState
            );
        }

        furtherPortal = 1 - furtherPortal;
        otherPortal = 1 - otherPortal;
    }
}

void renderPlanBuild(struct RenderPlan* renderPlan, struct Scene* scene, struct RenderState* renderState) {
    renderPropsInit(&renderPlan->stageProps[0], &scene->camera, (float)SCREEN_WD / (float)SCREEN_HT, renderState, scene->player.body.currentRoom);
    renderPlan->stageCount = 1;

    renderPlanFinishView(renderPlan, scene, &renderPlan->stageProps[0], renderState);
}

void renderPlanExecute(struct RenderPlan* renderPlan, struct Scene* scene, struct RenderState* renderState) {
    for (int i = renderPlan->stageCount - 1; i >= 0; --i) {
        struct RenderProps* current = &renderPlan->stageProps[i];

        if (!cameraApplyMatrices(renderState, &current->cameraMatrixInfo)) {
            return;
        }

        gSPViewport(renderState->dl++, current->viewport);
        gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, current->minX, current->minY, current->maxX, current->maxY);

        int portalIndex = (current->portalRenderType & PORTAL_RENDER_TYPE_SECOND_CLOSER) ? 1 : 0;
        
        for (int i = 0; i < 2; ++i) {
            if (current->portalRenderType & PORTAL_RENDER_TYPE_VISIBLE(portalIndex)) {
                float portalTransform[4][4];
                struct Portal* portal = &scene->portals[portalIndex];
                portalDetermineTransform(portal, portalTransform);

                if (current->portalRenderType & PORTAL_RENDER_TYPE_ENABLED(portalIndex)) {
                    // render the front portal cover
                    Mtx* matrix = renderStateRequestMatrices(renderState, 1);

                    if (!matrix) {
                        continue;;
                    }

                    guMtxF2L(portalTransform, matrix);
                    gSPMatrix(renderState->dl++, matrix, G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL);

                    gDPSetEnvColor(renderState->dl++, 255, 255, 255, portal->opacity < 0.0f ? 0 : (portal->opacity > 1.0f ? 255 : (u8)(portal->opacity * 255.0f)));
                    
                    if (portal->flags & PortalFlagsOddParity) {
                        gSPDisplayList(renderState->dl++, portal_portal_blue_face_model_gfx);
                        portalRenderScreenCover(current->clipper.nearPolygon, current->clipper.nearPolygonCount, current, renderState);
                        gDPPipeSync(renderState->dl++);

                        gSPDisplayList(renderState->dl++, portal_portal_blue_model_gfx);
                    } else {
                        gSPDisplayList(renderState->dl++, portal_portal_orange_face_model_gfx);
                        portalRenderScreenCover(current->clipper.nearPolygon, current->clipper.nearPolygonCount, current, renderState);
                        gDPPipeSync(renderState->dl++);

                        gSPDisplayList(renderState->dl++, portal_portal_orange_model_gfx);
                    }
                    
                    gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
                } else {
                    portalRenderCover(portal, portalTransform, renderState);
                }
            }

            portalIndex = 1 - portalIndex;
        }

        staticRender(&current->camera.transform, &current->cameraMatrixInfo.cullingInformation, current->visiblerooms, renderState);
    }
}