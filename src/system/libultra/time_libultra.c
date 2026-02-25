#include "system/time.h"

#include <ultra64.h>

Time timeGetTime() {
    return (Time)(osGetTime());
}

Time timeFromSeconds(float seconds) {
    return (Time)(OS_USEC_TO_CYCLES((u64)(seconds * 1000000.0f)));
}

uint64_t timeMicroseconds(Time time) {
    return (uint64_t)(OS_CYCLES_TO_USEC((OSTime)(time)));
}

uint64_t timeNanoseconds(Time time) {
    return (uint64_t)(OS_CYCLES_TO_NSEC((OSTime)(time)));
}
