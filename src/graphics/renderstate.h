#ifndef __RENDER_STATE_H__
#define __RENDER_STATE_H__

#include <ultra64.h>

#define MAX_DL_LENGTH           2000
#define MAX_ACTIVE_TRANSFORMS   100
#define MAX_DYNAMIC_LIGHTS      128

struct RenderState {
    Gfx glist[MAX_DL_LENGTH];
    Mtx matrices[MAX_ACTIVE_TRANSFORMS];
    Light lights[MAX_DYNAMIC_LIGHTS];
    Gfx* dl;
    unsigned short currentMatrix;
    unsigned short currentLight;
    unsigned currentChunkEnd;
};

void renderStateInit(struct RenderState* renderState);
Mtx* renderStateRequestMatrices(struct RenderState* renderState, unsigned count);
Light* renderStateRequestLights(struct RenderState* renderState, unsigned count);
void renderStateFlushCache(struct RenderState* renderState);
Gfx* renderStateAllocateDLChunk(struct RenderState* renderState, unsigned count);
Gfx* renderStateReplaceDL(struct RenderState* renderState, Gfx* nextDL);
Gfx* renderStateStartChunk(struct RenderState* renderState);
Gfx* renderStateEndChunk(struct RenderState* renderState, Gfx* chunkStart);

#endif