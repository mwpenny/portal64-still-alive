#include "frame_time.h"

#define FPS_TO_DELTA(fps) ((1.0f + FRAME_SKIP) / (float)(fps));

float gFixedDeltaTime = FPS_TO_DELTA(60);

static Time lastFrameStart;
Time gLastFrameTime;

void frameTimeInit(int fps) {
    gFixedDeltaTime = FPS_TO_DELTA(fps);
}

void frameTimeUpdate() {
    Time currTime = timeGetTime();
    gLastFrameTime = currTime - lastFrameStart;
    lastFrameStart = currTime;
}
