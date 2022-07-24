#ifndef __GRAPHICS_H__
#define __GRAPHICS_H__

#include <ultra64.h>
#include <sched.h>
#include "renderstate.h"
#include "defs.h"

#if HIGH_RES
#define SCREEN_WD   640
#define SCREEN_HT   480
#else
#define SCREEN_WD   320
#define SCREEN_HT   240
#endif

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

#endif