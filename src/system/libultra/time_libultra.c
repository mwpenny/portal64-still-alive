#include "system/time.h"

#include <ultra64.h>

float gTimePassed = 0.0f;
int gCurrentFrame = 0;
float gFixedDeltaTime = ((1.0f + FRAME_SKIP) / 60.0f);

OSMesgQueue timerQueue;
OSMesg      timerQueueBuf;

void timeInit() {
    osCreateMesgQueue(&timerQueue, &timerQueueBuf, 1);
}

Time timeGetTime() {
    return (Time)(osGetTime());
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
    (void) osRecvMesg(&timerQueue, NULL, OS_MESG_BLOCK);
}

void timeUpdateDelta() {
    OSTime currTime = osGetTime();
    gTimePassed = (float)OS_CYCLES_TO_USEC(currTime) / 1000000.0f;
    ++gCurrentFrame;
}

void timeSetFrameRate(int fps) {
    gFixedDeltaTime = ((1.0f + FRAME_SKIP) / (float)fps);
}