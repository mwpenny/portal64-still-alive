#ifndef _TIME_H
#define _TIME_H

#include <stdint.h>

typedef uint64_t Time;

extern float gFixedDeltaTime;
extern Time gLastFrameTime;

#define FRAME_SKIP  1
#define FIXED_DELTA_TIME    gFixedDeltaTime

void timeInit();
Time timeGetTime();
Time timeFromSeconds(float seconds);
uint64_t timeMicroseconds(Time time);
uint64_t timeNanoseconds(Time time);
void timeUSleep(uint64_t usec);
void timeUpdateFrameTime();
void timeSetFrameRate(int fps);

#endif