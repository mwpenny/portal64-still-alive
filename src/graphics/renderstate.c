#include "renderstate.h"

void renderStateInit(struct RenderState* renderState) {
    renderState->dl = renderState->glist;
    renderState->currentMatrix = 0;
    renderState->currentLight = 0;
    renderState->currentChunkEnd = MAX_DL_LENGTH;
}

Mtx* renderStateRequestMatrices(struct RenderState* renderState, unsigned count) {
    if (renderState->currentMatrix + count <= MAX_ACTIVE_TRANSFORMS) {
        Mtx* result = &renderState->matrices[renderState->currentMatrix];
        renderState->currentMatrix += count;
        return result;
    }

    return 0;
}

Light* renderStateRequestLights(struct RenderState* renderState, unsigned count) {
    if (renderState->currentLight + count <= MAX_ACTIVE_TRANSFORMS) {
        Light* result = &renderState->lights[renderState->currentLight];
        renderState->currentLight += count;
        return result;
    }

    return 0;
}

void renderStateFlushCache(struct RenderState* renderState) {
    osWritebackDCache(renderState, sizeof(struct RenderState));
}

Gfx* renderStateAllocateDLChunk(struct RenderState* renderState, unsigned count) {
    Gfx* result = &renderState->glist[renderState->currentChunkEnd - count];
    renderState->currentChunkEnd -= count;
    return result;
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