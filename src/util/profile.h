#ifndef __UTIL_PROFILE_H__
#define __UTIL_PROFILE_H__

#include "system/time.h"

#define profileStart() timeGetTime()
void profileEnd(Time startTime, int bin);

void profileReport();

#define MAX_PROFILE_BINS    8

#endif