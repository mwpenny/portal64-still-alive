#ifndef __FRAME_TIME_H__
#define __FRAME_TIME_H__

#include "system/time.h"

extern float gFixedDeltaTime;
extern Time  gLastFrameTime;

#define FRAME_SKIP          1
#define FIXED_DELTA_TIME    gFixedDeltaTime

void frameTimeInit(int fps);
void frameTimeUpdate();

#endif
