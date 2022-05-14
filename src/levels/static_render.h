#ifndef __STATIC_RENDER_H__
#define __STATIC_RENDER_H__

#include "level_definition.h"
#include "graphics/renderstate.h"
#include "scene/camera.h"

void staticRenderInit();

void staticRender(struct FrustrumCullingInformation* cullingInfo, struct RenderState* renderState);

int isOutsideFrustrum(struct FrustrumCullingInformation* frustrum, struct BoundingBoxs16* boundingBox);

#endif