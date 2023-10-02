#ifndef __UTIL_PROFILE_H__
#define __UTIL_PROFILE_H__

#include <ultra64.h>

#define profileStart() osGetTime()
void profileEnd(u64 startTime, int bin);

void profileReport();

#define MAX_PROFILE_BINS    8

#endif