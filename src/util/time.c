
#include "time.h"

float gTimePassed = 0.0f;
int gCurrentFrame = 0;
float gFixedDeltaTime = ((1.0f + FRAME_SKIP) / 60.0f);

void timeUpdateDelta() {
    OSTime currTime = osGetTime();
    gTimePassed = (float)OS_CYCLES_TO_USEC(currTime) / 1000000.0f;
    ++gCurrentFrame;
}

void timeSetFrameRate(int fps) {
    gFixedDeltaTime = ((1.0f + FRAME_SKIP) / (float)fps);
}