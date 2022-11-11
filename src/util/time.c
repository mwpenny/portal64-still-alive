
#include "time.h"

float gTimePassed = 0.0f;
int gCurrentFrame = 0;

void timeUpdateDelta() {
    OSTime currTime = osGetTime();
    gTimePassed = (float)OS_CYCLES_TO_USEC(currTime) / 1000000.0f;
    ++gCurrentFrame;
}