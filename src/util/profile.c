#include "profile.h"

#if PORTAL64_WITH_DEBUGGER
#include "debugger/debug.h"
#endif

#define MAX_PROFILE_BINS 8

struct ProfileData {
    uint64_t lastReportStart;
    uint64_t timeAccumulation[MAX_PROFILE_BINS];
};

static struct ProfileData sProfileData;

void profileEnd(Time startTime, int bin) {
    sProfileData.timeAccumulation[bin] += timeMicroseconds(timeGetTime() - startTime);
}

void profileReport() {
#if PORTAL64_WITH_DEBUGGER
    uint64_t reportStartTime = timeMicroseconds(timeGetTime());

    sProfileData.lastReportStart = reportStartTime - sProfileData.lastReportStart;
    // debug_dumpbinary(&sProfileData, sizeof(struct ProfileData));

    for (int i = 0; i < MAX_PROFILE_BINS; ++i) {
        sProfileData.timeAccumulation[i] = 0;
    }

    sProfileData.lastReportStart = reportStartTime;
#endif
}
