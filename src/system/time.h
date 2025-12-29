#ifndef __TIME_H__
#define __TIME_H__

#include <stdint.h>

typedef uint64_t Time;

void timeInit();
Time timeGetTime();
Time timeFromSeconds(float seconds);
uint64_t timeMicroseconds(Time time);
uint64_t timeNanoseconds(Time time);
void timeUSleep(uint64_t usec);

#endif
