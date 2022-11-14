#ifndef __SCENE_RENDER_PLAN_H__
#define __SCENE_RENDER_PLAN_H__

#include "./scene.h"

#define MAX_PORTAL_STEPS    6

struct RenderPlan {
    struct RenderProps stageProps[MAX_PORTAL_STEPS];
    short stageCount;
};

void renderPlanBuild(struct RenderPlan* renderPlan, struct Scene* scene, struct RenderState* renderState);

void renderPlanExecute(struct RenderPlan* renderPlan, struct Scene* scene, struct RenderState* renderState);

#endif