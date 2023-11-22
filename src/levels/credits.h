#ifndef __LEVELS_CREDITS_H__
#define __LEVELS_CREDITS_H__

#include "../graphics/graphics.h"

struct Credits {
    float time;
};

void creditsInit(struct Credits* credits);
void creditsUpdate(struct Credits* credits);
void creditsRender(void* data, struct RenderState* renderState, struct GraphicsTask* task);

#endif