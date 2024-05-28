#include "profile.h"

#ifdef PORTAL64_WITH_DEBUGGER
#include "../debugger/serial.h"
#endif

struct ProfileData {
    uint64_t lastReportStart;
    uint64_t timeAccumulation[MAX_PROFILE_BINS];
};

struct ProfileData gProfileData;

void profileEnd(Time startTime, int bin) {
    gProfileData.timeAccumulation[bin] += timeMicroseconds(timeGetTime() - startTime);
}

void profileReport() {
#ifdef PORTAL64_WITH_DEBUGGER
    uint64_t reportStartTime = timeMicroseconds(timeGetTime());

    gProfileData.lastReportStart = reportStartTime - gProfileData.lastReportStart;
    // gdbSendMessage(GDBDataTypeRawBinary, (char*)&gProfileData, sizeof(struct ProfileData));

    for (int i = 0; i < MAX_PROFILE_BINS; ++i) {
        gProfileData.timeAccumulation[i] = 0;
    }

    gProfileData.lastReportStart = reportStartTime;
#endif
}