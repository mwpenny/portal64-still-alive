#include "frame_time.h"

float gFixedDeltaTime = ((1.0f + FRAME_SKIP) / 60.0f);

static Time lastFrameStart;
Time gLastFrameTime;

void frameTimeUpdate() {
    Time currTime = timeGetTime();
    gLastFrameTime = currTime - lastFrameStart;
    lastFrameStart = currTime;
}

void frameTimeSetFixedDelta(int fps) {
    gFixedDeltaTime = ((1.0f + FRAME_SKIP) / (float)fps);
}
