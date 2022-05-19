#ifndef __STATIC_RENDER_H__
#define __STATIC_RENDER_H__

#include "level_definition.h"
#include "graphics/renderstate.h"
#include "scene/camera.h"
#include "../scene/dynamic_scene.h"

void staticRenderInit();

int staticRenderSorkKeyFromMaterial(int materialIndex, float distanceScaled);
void staticRender(struct FrustrumCullingInformation* cullingInfo, struct RenderState* renderState);

#endif