#ifndef _TIME_H
#define _TIME_H

#include <ultra64.h>

extern float gTimePassed;
extern OSTime gLastTime;
extern int gCurrentFrame;

#define FRAME_SKIP  1
#define FIXED_DELTA_TIME    ((1.0f + FRAME_SKIP) / 60.0f)

void timeUpdateDelta();

#endif