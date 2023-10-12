#include "profile.h"

#ifdef PORTAL64_WITH_DEBUGGER
#include "../debugger/serial.h"
#endif

struct ProfileData {
    u64 lastReportStart;
    u64 timeAccumulation[MAX_PROFILE_BINS];
};

struct ProfileData gProfileData;

void profileEnd(u64 startTime, int bin) {
    gProfileData.timeAccumulation[bin] += OS_CYCLES_TO_USEC(osGetTime() - startTime);
}

void profileReport() {
#ifdef PORTAL64_WITH_DEBUGGER
    OSTime reportStartTime = osGetTime();

    gProfileData.lastReportStart = OS_CYCLES_TO_USEC(reportStartTime - gProfileData.lastReportStart);
    // gdbSendMessage(GDBDataTypeRawBinary, (char*)&gProfileData, sizeof(struct ProfileData));

    for (int i = 0; i < MAX_PROFILE_BINS; ++i) {
        gProfileData.timeAccumulation[i] = 0;
    }

    gProfileData.lastReportStart = reportStartTime;
#endif
}