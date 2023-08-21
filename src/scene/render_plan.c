
#include "render_plan.h"

#include "../graphics/screen_clipper.h"
#include "../physics/collision_scene.h"
#include "../levels/static_render.h"

#include "../util/memory.h"
#include "../math/mathf.h"
#include "../math/matrix.h"

#include "portal_render.h"
#include "../scene/dynamic_scene.h"
#include "../levels/levels.h"

#include "../build/assets/models/portal/portal_blue.h"
#include "../build/assets/models/portal/portal_blue_face.h"
#include "../build/assets/models/portal/portal_orange.h"
#include "../build/assets/models/portal/portal_orange_face.h"

#define MIN_VP_WIDTH 64
#define CAMERA_CLIPPING_RADIUS  0.2f
#define PORTAL_CLIPPING_OFFSET  0.1f

void renderPropsInit(struct RenderProps* props, struct Camera* camera, float aspectRatio, struct RenderState* renderState, u16 roomIndex) {
    props->camera = *camera;
    props->aspectRatio = aspectRatio;

    cameraSetupMatrices(camera, renderState, aspectRatio, &fullscreenViewport, 1, &props->cameraMatrixInfo);

    props->viewport = &fullscreenViewport;
    props->currentDepth = STARTING_RENDER_DEPTH;
    props->exitPortalIndex = NO_PORTAL;
    props->fromRoom = roomIndex;
    props->parentStageIndex = -1;

    props->clippingPortalIndex = -1;

    props->minX = 0;
    props->minY = 0;
    props->maxX = SCREEN_WD;
    props->maxY = SCREEN_HT;

    props->previousProperties = NULL;
    props->nextProperites[0] = NULL;
    props->nextProperites[1] = NULL;

    props->portalRenderType = 0;
    props->visiblerooms = 0;
}

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

    if (!viewport) {
        return NULL;
    }

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

void renderPlanFinishView(struct RenderPlan* renderPlan, struct Scene* scene, struct RenderProps* properties, struct RenderState* renderState);


#define CALC_SCREEN_SPACE(clip_space, screen_size) ((clip_space + 1.0f) * ((screen_size) / 2))

int renderPlanPortal(struct RenderPlan* renderPlan, struct Scene* scene, struct RenderProps* current, int portalIndex, struct RenderProps** prevSiblingPtr, struct RenderState* renderState) {
    int exitPortalIndex = 1 - portalIndex;
    struct Portal* portal = &scene->portals[portalIndex];

    struct Vector3 forward = gForward;
    if (!(portal->flags & PortalFlagsOddParity)) {
        forward.z = -1.0f;
    }

    struct Vector3 worldForward;
    quatMultVector(&portal->rigidBody.transform.rotation, &forward, &worldForward);

    struct Vector3 offsetFromCamera;
    vector3Sub(&current->camera.transform.position, &portal->rigidBody.transform.position, &offsetFromCamera);

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

    struct ScreenClipper clipper;

    screenClipperInitWithCamera(&clipper, &current->camera, (float)SCREEN_WD / (float)SCREEN_HT, portalTransform);
    struct Box2D clippingBounds;
    screenClipperBoundingPoints(&clipper, gPortalOutline, sizeof(gPortalOutline) / sizeof(*gPortalOutline), &clippingBounds);

    if (clipper.nearPolygonCount) {
        renderPlan->nearPolygonCount = clipper.nearPolygonCount;
        memCopy(renderPlan->nearPolygon, clipper.nearPolygon, sizeof(struct Vector2s16) * clipper.nearPolygonCount);
        renderPlan->clippedPortalIndex = portalIndex;
    }

    next->minX = CALC_SCREEN_SPACE(clippingBounds.min.x, SCREEN_WD);
    next->maxX = CALC_SCREEN_SPACE(clippingBounds.max.x, SCREEN_WD);
    next->minY = CALC_SCREEN_SPACE(-clippingBounds.max.y, SCREEN_HT);
    next->maxY = CALC_SCREEN_SPACE(-clippingBounds.min.y, SCREEN_HT);

    next->minX = MAX(next->minX, current->minX);
    next->maxX = MIN(next->maxX, current->maxX);
    next->minY = MAX(next->minY, current->minY);
    next->maxY = MIN(next->maxY, current->maxY);

    struct RenderProps* prevSibling = prevSiblingPtr ? *prevSiblingPtr : NULL;

    if (prevSibling) {
        int topDiff = 0;
        int bottomDiff = 0;
        int leftDiff = 0;
        int rightDiff = 0;

        // if top left overlapping
        if ((next->minX > prevSibling->minX) && (next->minX < prevSibling->maxX) && (next->minY > prevSibling->minY) && (next->minY < prevSibling->maxY)){
            topDiff = MAX(abs(prevSibling->maxY-next->minY), topDiff);
            leftDiff = MAX(abs(prevSibling->maxX-next->minX), leftDiff);
        }

        // if top right overlapping
        if ((next->maxX > prevSibling->minX) && (next->maxX < prevSibling->maxX) && (next->minY > prevSibling->minY) && (next->minY < prevSibling->maxY)){
            topDiff = MAX(abs(prevSibling->maxY-next->minY), topDiff);
            rightDiff = MAX(abs(next->maxX -prevSibling->minX), rightDiff);
        }

        // if bottom left overlapping
        if ((next->minX > prevSibling->minX) && (next->minX < prevSibling->maxX) && (next->maxY > prevSibling->minY) && (next->maxY < prevSibling->maxY)){
            bottomDiff = MAX(abs(next->maxY-prevSibling->minY), bottomDiff);
            leftDiff = MAX(abs(prevSibling->maxX-next->minX), leftDiff);
        }

        // if bottom right overlapping
        if ((next->maxX > prevSibling->minX) && (next->maxX < prevSibling->maxX) && (next->maxY > prevSibling->minY) && (next->maxY < prevSibling->maxY)){
            bottomDiff = MAX(abs(next->maxY-prevSibling->minY), bottomDiff);
            rightDiff = MAX(abs(next->maxX -prevSibling->minX), rightDiff);
        }

        //shouldnt draw at all if fully overlapped
        if (rightDiff && leftDiff && topDiff && bottomDiff){
            return 0;
        }
        //cut nothing if no overlap
        else if (!rightDiff && !leftDiff && !topDiff && !bottomDiff){
            //do nothing
        }
        //should only cut out top portion
        else if (rightDiff && leftDiff && topDiff && !bottomDiff){
            next->minY += topDiff;
        }
        //should only cut out bottom portion
        else if (rightDiff && leftDiff && !topDiff && bottomDiff){
            next->maxY -= bottomDiff;
        }
        //should only cut out right portion
        else if (rightDiff && !leftDiff && topDiff && bottomDiff){
            next->maxX -= rightDiff;
        }
        //should only cut out left portion
        else if (!rightDiff && leftDiff && topDiff && bottomDiff){
            next->minX += leftDiff;
        }
        // only one corner is overlapping so cut the larger side
        else{
            if (MAX(rightDiff, leftDiff) > MAX(topDiff, bottomDiff)){
                next->minY += topDiff;
                next->maxY -= bottomDiff;
            }
            else{
                next->minX += leftDiff;
                next->maxX -= rightDiff;
            }
        }
    }

    if (next->minX >= next->maxX || next->minY >= next->maxY) {
        return 0;
    }

    struct Transform* fromPortal = &scene->portals[portalIndex].rigidBody.transform;
    struct Transform* exitPortal = &scene->portals[exitPortalIndex].rigidBody.transform;

    struct Transform otherInverse;
    transformInvert(fromPortal, &otherInverse);
    struct Transform portalCombined;
    transformConcat(exitPortal, &otherInverse, &portalCombined);

    next->camera = current->camera;
    next->camera.farPlane = DEFAULT_FAR_PLANE * SCENE_SCALE;
    next->camera.nearPlane = DEFAULT_NEAR_PLANE * SCENE_SCALE;
    next->aspectRatio = current->aspectRatio;
    transformConcat(&portalCombined, &current->camera.transform, &next->camera.transform);

    struct Vector3 portalOffset;
    vector3Sub(&exitPortal->position, &next->camera.transform.position, &portalOffset);

    struct Vector3 cameraForward;
    quatMultVector(&next->camera.transform.rotation, &gForward, &cameraForward);

    next->camera.nearPlane = (-vector3Dot(&portalOffset, &cameraForward)) * SCENE_SCALE - SCENE_SCALE * PORTAL_COVER_HEIGHT * 0.5f;

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

    if (!cameraSetupMatrices(&next->camera, renderState, next->aspectRatio, next->viewport, 1, &next->cameraMatrixInfo)) {
        return flags;
    }

    // set the near clipping plane to be the exit portal surface
    quatMultVector(&exitPortal->rotation, &gForward, &next->cameraMatrixInfo.cullingInformation.clippingPlanes[4].normal);
    if (portalIndex == 1) {
        vector3Negate(&next->cameraMatrixInfo.cullingInformation.clippingPlanes[4].normal, &next->cameraMatrixInfo.cullingInformation.clippingPlanes[4].normal);
    }
    next->cameraMatrixInfo.cullingInformation.clippingPlanes[4].d = -(vector3Dot(&next->cameraMatrixInfo.cullingInformation.clippingPlanes[4].normal, &exitPortal->position) + 0.01f) * SCENE_SCALE;

    next->clippingPortalIndex = -1;

    next->exitPortalIndex = exitPortalIndex;
    next->fromRoom = gCollisionScene.portalRooms[next->exitPortalIndex];
    next->parentStageIndex = current - renderPlan->stageProps;

    ++renderPlan->stageCount;

    next->previousProperties = current;
    current->nextProperites[portalIndex] = next;
    next->nextProperites[0] = NULL;
    next->nextProperites[1] = NULL;

    next->visiblerooms = 0;

    next->portalRenderType = 0;

    *prevSiblingPtr = next;

    renderPlanFinishView(renderPlan, scene, next, renderState);

    return flags | PORTAL_RENDER_TYPE_ENABLED(portalIndex);
}

#define MIN_FAR_PLANE   (5.0f * SCENE_SCALE)
#define FAR_PLANE_EXTRA  2.0f

void renderPlanDetermineFarPlane(struct Ray* cameraRay, struct RenderProps* properties) {

    float furthestDistance = (worldMaxDistanceInDirection(&gCurrentLevel->world, cameraRay, properties->visiblerooms) + FAR_PLANE_EXTRA) * SCENE_SCALE;

    if (furthestDistance < MIN_FAR_PLANE) {
        properties->camera.farPlane = MIN_FAR_PLANE;
    } else {
        properties->camera.farPlane = MIN(properties->camera.farPlane, furthestDistance);
    }
}

void renderPlanFinishView(struct RenderPlan* renderPlan, struct Scene* scene, struct RenderProps* properties, struct RenderState* renderState) {
    
    staticRenderDetermineVisibleRooms(&properties->cameraMatrixInfo.cullingInformation, properties->fromRoom, &properties->visiblerooms);

    struct Ray cameraRay;
    quatMultVector(&properties->camera.transform.rotation, &gForward, &cameraRay.dir);
    cameraRay.origin = properties->camera.transform.position;
    vector3Negate(&cameraRay.dir, &cameraRay.dir);

    renderPlanDetermineFarPlane(&cameraRay, properties);

    cameraSetupMatrices(&properties->camera, renderState, properties->aspectRatio, properties->viewport, 0, &properties->cameraMatrixInfo);

    int closerPortal = vector3DistSqrd(&properties->camera.transform.position, &scene->portals[0].rigidBody.transform.position) < vector3DistSqrd(&properties->camera.transform.position, &scene->portals[1].rigidBody.transform.position) ? 0 : 1;
    int otherPortal = 1 - closerPortal;
    
    if (closerPortal) {
        properties->portalRenderType |= PORTAL_RENDER_TYPE_SECOND_CLOSER;
    }

    struct RenderProps* prevSibling = NULL;

    float furthestPortal = 0.0f;

    for (int i = 0; i < 2; ++i) {
        if (gCollisionScene.portalTransforms[closerPortal] && 
            properties->exitPortalIndex != closerPortal && 
            staticRenderIsRoomVisible(properties->visiblerooms, gCollisionScene.portalRooms[closerPortal])) {
            int planResult = renderPlanPortal(
                renderPlan,
                scene,
                properties,
                closerPortal,
                &prevSibling,
                renderState
            );

            properties->portalRenderType |= planResult;

            if (planResult) {
                float portalDistance = rayDetermineDistance(&cameraRay, &gCollisionScene.portalTransforms[closerPortal]->position) + 1.0f;

                furthestPortal = MAX(furthestPortal, portalDistance);
            }
        }

        closerPortal = 1 - closerPortal;
        otherPortal = 1 - otherPortal;
    }

    properties->maxZOverlap = matrixNormalizedZValue(-furthestPortal * SCENE_SCALE, properties->camera.nearPlane, properties->camera.farPlane);
}

void renderPlanAdjustViewportDepth(struct RenderPlan* renderPlan) {
    float depthWeight[STARTING_RENDER_DEPTH + 1];

    for (int i = 0; i <= STARTING_RENDER_DEPTH; ++i) {
        depthWeight[i] = 0.0f;
    }

    for (int i = 0; i < renderPlan->stageCount; ++i) {
        struct RenderProps* current = &renderPlan->stageProps[i];

        float depth = current->camera.farPlane - current->camera.nearPlane;

        depthWeight[current->currentDepth] = MAX(depthWeight[current->currentDepth], depth);
    }

    float totalWeight = 0.0f;

    for (int i = 0; i <= STARTING_RENDER_DEPTH; ++i) {
        totalWeight += depthWeight[i];
    }

    // give the main view a larger slice of the depth buffer
    totalWeight += depthWeight[STARTING_RENDER_DEPTH];
    depthWeight[STARTING_RENDER_DEPTH] *= 2.0f;

    float scale = (float)G_MAXZ / totalWeight;

    short zBufferBoundary[STARTING_RENDER_DEPTH + 2];

    zBufferBoundary[STARTING_RENDER_DEPTH + 1] = 0;

    for (int i = STARTING_RENDER_DEPTH; i >= 0; --i) {
        zBufferBoundary[i] = (short)(scale * depthWeight[i]) + zBufferBoundary[i + 1];

        zBufferBoundary[i] = MIN(zBufferBoundary[i], G_MAXZ);
    }

    for (int i = 0; i < renderPlan->stageCount; ++i) {
        struct RenderProps* current = &renderPlan->stageProps[i];
        short minZ = zBufferBoundary[current->currentDepth + 1];
        short maxZ = zBufferBoundary[current->currentDepth];

        current->viewport->vp.vscale[2] = (maxZ - minZ) >> 1;
        current->viewport->vp.vtrans[2] = (maxZ + minZ) >> 1;
    }
}

void renderPlanBuild(struct RenderPlan* renderPlan, struct Scene* scene, struct RenderState* renderState) {
    renderPropsInit(&renderPlan->stageProps[0], &scene->camera, (float)SCREEN_WD / (float)SCREEN_HT, renderState, scene->player.body.currentRoom);
    renderPlan->stageCount = 1;
    renderPlan->clippedPortalIndex = -1;
    renderPlan->nearPolygonCount = 0;

    renderPlanFinishView(renderPlan, scene, &renderPlan->stageProps[0], renderState);

    renderPlanAdjustViewportDepth(renderPlan);
}

#define MIN_FOG_DISTANCE 1.0f
#define MAX_FOG_DISTANCE 2.5f

int fogIntValue(float floatValue) {
    if (floatValue < -1.0) {
        return 0;
    }

    if (floatValue > 1.0) {
        return 1000;
    }

    return (int)((floatValue + 1.0f) * 500.0f);
}

void renderPlanExecute(struct RenderPlan* renderPlan, struct Scene* scene, Mtx* staticTransforms, struct RenderState* renderState) {
    struct DynamicRenderDataList* dynamicList = dynamicRenderListNew(renderState, MAX_DYNAMIC_SCENE_OBJECTS);

    for (int i = 0; i < renderPlan->stageCount; ++i) {
        dynamicRenderAddStage(dynamicList, renderPlan->stageProps[i].exitPortalIndex, renderPlan->stageProps[i].parentStageIndex);
    }

    dynamicRenderListPopulate(dynamicList, renderPlan->stageProps, renderPlan->stageCount, renderState);

    for (int stageIndex = renderPlan->stageCount - 1; stageIndex >= 0; --stageIndex) {
        struct RenderProps* current = &renderPlan->stageProps[stageIndex];

        if (!cameraApplyMatrices(renderState, &current->cameraMatrixInfo)) {
            return;
        }

        gSPViewport(renderState->dl++, current->viewport);
        gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, current->minX, current->minY, current->maxX, current->maxY);

        float lerpMin = cameraClipDistance(&current->camera, MIN_FOG_DISTANCE);
        float lerpMax = cameraClipDistance(&current->camera, MAX_FOG_DISTANCE);

        int fogMin = fogIntValue(lerpMin);
        int fogMax = fogIntValue(lerpMax);

        if (fogMax <= 0) {
            fogMax = 1;
        }

        if (fogMin >= 1000) {
            fogMin = 999;
        }
        
        gSPFogPosition(renderState->dl++, fogMin, fogMax);

        int portalIndex = (current->portalRenderType & PORTAL_RENDER_TYPE_SECOND_CLOSER) ? 1 : 0;
        
        for (int i = 0; i < 2; ++i) {
            if (current->portalRenderType & PORTAL_RENDER_TYPE_VISIBLE(portalIndex)) {
                float portalTransform[4][4];
                struct Portal* portal = &scene->portals[portalIndex];
                portalDetermineTransform(portal, portalTransform);

                struct RenderProps* portalProps = current->nextProperites[portalIndex];

                if (portalProps && current->portalRenderType & PORTAL_RENDER_TYPE_ENABLED(portalIndex)) {
                    // render the front portal cover
                    Mtx* matrix = renderStateRequestMatrices(renderState, 1);

                    if (!matrix) {
                        continue;;
                    }

                    guMtxF2L(portalTransform, matrix);
                    gSPMatrix(renderState->dl++, matrix, G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL);

                    gDPSetEnvColor(renderState->dl++, 255, 255, 255, portal->opacity < 0.0f ? 0 : (portal->opacity > 1.0f ? 255 : (u8)(portal->opacity * 255.0f)));
                    
                    Gfx* faceModel;
                    Gfx* portalModel;

                    if (portal->flags & PortalFlagsOddParity) {
                        faceModel = portal_portal_blue_face_model_gfx;
                        portalModel = portal_portal_blue_model_gfx;
                    } else {
                        faceModel = portal_portal_orange_face_model_gfx;
                        portalModel = portal_portal_orange_model_gfx;
                    }

                    gSPDisplayList(renderState->dl++, faceModel);
                    if (current->previousProperties == NULL && portalIndex == renderPlan->clippedPortalIndex && renderPlan->nearPolygonCount) {
                        portalRenderScreenCover(renderPlan->nearPolygon, renderPlan->nearPolygonCount, current, renderState);
                    }
                    gDPPipeSync(renderState->dl++);

                    gSPDisplayList(renderState->dl++, portalModel);
                    
                    gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
                } else {
                    portalRenderCover(portal, portalTransform, renderState);
                }
            }

            portalIndex = 1 - portalIndex;
        }

        staticRender(
            &current->camera.transform, 
            &current->cameraMatrixInfo.cullingInformation, 
            current->visiblerooms, 
            dynamicList, 
            stageIndex, 
            staticTransforms, 
            renderState
        );
    }

    dynamicRenderListFree(dynamicList);
}