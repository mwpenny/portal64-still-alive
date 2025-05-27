#ifndef __DEBUG_SCENE_H__
#define __DEBUG_SCENE_H__

#include "graphics/renderstate.h"
#include "render_plan.h"
#include "scene.h"

void debugSceneInit(struct Scene* scene);
void debugSceneUpdate(struct Scene* scene);
void debugSceneRender(struct Scene* scene, struct RenderState* renderState, struct RenderPlan* renderPlan);

#endif
