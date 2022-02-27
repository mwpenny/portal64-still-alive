
#include "time.h"

#define MAX_DELTA_TIME      0.1f

float gTimePassed = 0.0f;
float gTimeDelta;
OSTime gLastTime;

void timeUpdateDelta() {
    OSTime currTime = osGetTime();
    OSTime timeDelta = currTime - gLastTime;

    gTimePassed = (float)OS_CYCLES_TO_USEC(currTime) / 1000000.0f;
    gTimeDelta = (float)OS_CYCLES_TO_USEC(timeDelta) / 1000000.0f;

    if (gTimeDelta > MAX_DELTA_TIME) {
        gTimeDelta = MAX_DELTA_TIME;
    }

    gLastTime = currTime;

}