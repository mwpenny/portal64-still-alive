#ifndef _TIME_H
#define _TIME_H

#include <stdint.h>

extern float gTimePassed;
extern int gCurrentFrame;
extern float gFixedDeltaTime;

#define FRAME_SKIP  1
#define FIXED_DELTA_TIME    gFixedDeltaTime

void timeInit();
void timeUSleep(uint64_t usec);
void timeUpdateDelta();
void timeSetFrameRate(int fps);

#endif