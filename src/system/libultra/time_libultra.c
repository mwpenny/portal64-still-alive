#include "system/time.h"

#include <ultra64.h>

static OSMesgQueue timerQueue;
static OSMesg      timerQueueBuf;

void timeInit() {
    osCreateMesgQueue(&timerQueue, &timerQueueBuf, 1);
}

Time timeGetTime() {
    return (Time)(osGetTime());
}

Time timeFromSeconds(float seconds) {
    return (Time)(OS_USEC_TO_CYCLES((u64)(seconds * 1000000.0f)));
}

uint64_t timeMicroseconds(Time time) {
    return (uint64_t)(OS_CYCLES_TO_USEC((OSTime)(time)));
}

uint64_t timeNanoseconds(Time time) {
    return (uint64_t)(OS_CYCLES_TO_NSEC((OSTime)(time)));
}

void timeUSleep(uint64_t usec) {
    OSTimer timer;
    OSTime  countdown = OS_USEC_TO_CYCLES((u64)(usec));

    osSetTimer(&timer, countdown, 0, &timerQueue, 0);
    osRecvMesg(&timerQueue, NULL, OS_MESG_BLOCK);
}
