#include "renderstate.h"

void renderStateInit(struct RenderState* renderState, u16* framebuffer, u16* depthBuffer) {
    renderState->dl = renderState->glist;
    renderState->currentMemoryChunk = &renderState->glist[MAX_DL_LENGTH + MAX_RENDER_STATE_MEMORY_CHUNKS];
    renderState->framebuffer = framebuffer;
    renderState->depthBuffer = depthBuffer;
}

void* renderStateRequestMemory(struct RenderState* renderState, unsigned size) {
    unsigned memorySlots = (size + 7) >> 3;

    Gfx* result = renderState->currentMemoryChunk - memorySlots;

    // display list grows up, allocated memory grows down
    if (result <= renderState->dl) {
        return 0;
    }

    renderState->currentMemoryChunk = result;

    return result;
}

Mtx* renderStateRequestMatrices(struct RenderState* renderState, unsigned count) {
    return renderStateRequestMemory(renderState, sizeof(Mtx) * count);
}

Light* renderStateRequestLights(struct RenderState* renderState, unsigned count) {
    return renderStateRequestMemory(renderState, sizeof(Light) * count);
}

Vp* renderStateRequestViewport(struct RenderState* renderState) {
    return renderStateRequestMemory(renderState, sizeof(Vp));
}

Vtx* renderStateRequestVertices(struct RenderState* renderState, unsigned count) {
    return renderStateRequestMemory(renderState, sizeof(Vtx) * count);
}

LookAt* renderStateRequestLookAt(struct RenderState* renderState) {
    return renderStateRequestMemory(renderState, sizeof(LookAt));
}

void renderStateFlushCache(struct RenderState* renderState) {
    osWritebackDCache(renderState, sizeof(struct RenderState));
}

Gfx* renderStateAllocateDLChunk(struct RenderState* renderState, unsigned count) {
    return renderStateRequestMemory(renderState, sizeof(Gfx) * count);
}

Gfx* renderStateReplaceDL(struct RenderState* renderState, Gfx* nextDL) {
    Gfx* result = renderState->dl;
    renderState->dl = nextDL;
    return result;
}

Gfx* renderStateStartChunk(struct RenderState* renderState) {
    return renderState->dl;    
}

Gfx* renderStateEndChunk(struct RenderState* renderState, Gfx* chunkStart) {
    Gfx* newChunk = renderStateAllocateDLChunk(renderState, (renderState->dl - chunkStart) + 1);
    Gfx* copyDest = newChunk;
    Gfx* copySrc = chunkStart;

    while (copySrc < renderState->dl) {
        *copyDest = *copySrc;
        ++copyDest;
        ++copySrc;
    }

    gSPEndDisplayList(copyDest++);

    renderState->dl = chunkStart;

    return newChunk;
}

int renderStateMaxDLCount(struct RenderState* renderState) {
    return renderState->currentMemoryChunk - renderState->glist;
}

void renderStateInlineBranch(struct RenderState* renderState, Gfx* dl) {
    while (_SHIFTR(dl->words.w0, 24, 8) != G_ENDDL) {
        *renderState->dl++ = *dl++;
    }
}