#ifndef _TIME_H
#define _TIME_H

#include <ultra64.h>

extern float gTimePassed;
extern OSTime gLastTime;
extern int gCurrentFrame;
extern float gFixedDeltaTime;

#define FRAME_SKIP  1
#define FIXED_DELTA_TIME    gFixedDeltaTime

void timeUpdateDelta();
void timeSetFrameRate(int fps);

#endif