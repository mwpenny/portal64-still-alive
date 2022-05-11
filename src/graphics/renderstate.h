#ifndef __RENDER_STATE_H__
#define __RENDER_STATE_H__

#include <ultra64.h>

#define MAX_DL_LENGTH           2000
#define MAX_RENDER_STATE_MEMORY 6400
#define MAX_RENDER_STATE_MEMORY_CHUNKS (MAX_RENDER_STATE_MEMORY / sizeof(u64))
#define MAX_DYNAMIC_LIGHTS      128

struct RenderState {
    Gfx glist[MAX_DL_LENGTH];
    u64 renderStateMemory[MAX_RENDER_STATE_MEMORY_CHUNKS];
    Gfx* dl;
    unsigned short currentMemoryChunk;
    unsigned short currentChunkEnd;
};

void renderStateInit(struct RenderState* renderState);
Mtx* renderStateRequestMatrices(struct RenderState* renderState, unsigned count);
Light* renderStateRequestLights(struct RenderState* renderState, unsigned count);
Vp* renderStateRequestViewport(struct RenderState* renderState);
void renderStateFlushCache(struct RenderState* renderState);
Gfx* renderStateAllocateDLChunk(struct RenderState* renderState, unsigned count);
Gfx* renderStateReplaceDL(struct RenderState* renderState, Gfx* nextDL);
Gfx* renderStateStartChunk(struct RenderState* renderState);
Gfx* renderStateEndChunk(struct RenderState* renderState, Gfx* chunkStart);

#endif