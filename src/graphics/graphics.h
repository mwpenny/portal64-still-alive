#ifndef __GRAPHICS_H__
#define __GRAPHICS_H__

#include <sched.h>
#include <ultra64.h>

#include "renderstate.h"

struct GraphicsTask {
    struct RenderState renderState;
    OSScTask task;
    OSScMsg msg;
    u16 *framebuffer;
    u16 taskIndex;
};

extern struct GraphicsTask gGraphicsTasks[2];
extern Vp fullscreenViewport;

extern void* gLevelSegment;

#define GET_GFX_TYPE(gfx)       (_SHIFTR((gfx)->words.w0, 24, 8))

typedef void (*GraphicsCallback)(void* data, struct RenderState* renderState, struct GraphicsTask* task);

u16* graphicsLayoutScreenBuffers(u16* memoryEnd);
void graphicsCreateTask(struct GraphicsTask* targetTask, GraphicsCallback callback, void* data);

void graphicsTaskClearZBuffer(struct GraphicsTask* task, int minX, int minY, int maxX, int maxY);

#endif