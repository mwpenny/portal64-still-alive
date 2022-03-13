#ifndef _TIME_H
#define _TIME_H

#include <ultra64.h>

extern float gTimePassed;
extern OSTime gLastTime;

#define FIXED_DELTA_TIME    (1.0f / 60.0f)

void timeUpdateDelta();

#endif