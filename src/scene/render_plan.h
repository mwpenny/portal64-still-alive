#ifndef __SCENE_RENDER_PLAN_H__
#define __SCENE_RENDER_PLAN_H__

#include "./scene.h"
#include "../graphics/screen_clipper.h"

#define DEFAULT_FAR_PLANE       30.0f
#define DEFAULT_NEAR_PLANE      0.125f

#define MAX_PORTAL_STEPS    6

struct RenderProps {
    struct Camera camera;
    float aspectRatio;

    struct CameraMatrixInfo cameraMatrixInfo; 

    Vp* viewport;

    float maxZOverlap;

    u8 currentDepth;
    u8 exitPortalIndex;
    s8 clippingPortalIndex;
    u8 portalRenderType;

    s8 parentStageIndex;

    u16 fromRoom;

    short minX;
    short minY;
    short maxX;
    short maxY;

    u64 visiblerooms;

    struct RenderProps* previousProperties;
    struct RenderProps* nextProperites[2];
};

struct RenderPlan {
    struct RenderProps stageProps[MAX_PORTAL_STEPS];
    short stageCount;
    short clippedPortalIndex;
    short nearPolygonCount;
    struct Vector2s16 nearPolygon[MAX_NEAR_POLYGON_SIZE];
};

void renderPlanBuild(struct RenderPlan* renderPlan, struct Scene* scene, struct RenderState* renderState);

void renderPlanExecute(struct RenderPlan* renderPlan, struct Scene* scene, Mtx* staticTransforms, struct RenderState* renderState);

#endif