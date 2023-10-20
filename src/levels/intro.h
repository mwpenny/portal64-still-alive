#ifndef __LEVELS_INTRO_H__
#define __LEVELS_INTRO_H__

#include "../graphics/graphics.h"

struct Intro {
    float time;
    u64* valveImage;
};

void introInit(struct Intro* intro);
void introUpdate(struct Intro* intro);
void introRender(void* data, struct RenderState* renderState, struct GraphicsTask* task);

#endif