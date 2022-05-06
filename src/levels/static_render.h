#ifndef __STATIC_RENDER_H__
#define __STATIC_RENDER_H__

#include "level_definition.h"
#include "graphics/renderstate.h"

void staticRenderInit();

void staticRender(struct RenderState* renderState);

#endif