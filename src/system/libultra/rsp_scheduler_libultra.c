#include "rsp_scheduler_libultra.h"

#include "threads_libultra.h"

static OSSched  sScheduler;
static u64      sSchedulerStack[OS_SC_STACKSIZE / sizeof(u64)];

void rspSchedulerInit(u8 viMode) {
    osCreateScheduler(
        &sScheduler,
        sSchedulerStack + (OS_SC_STACKSIZE / sizeof(u64)),
        RSP_SCHEDULER_THREAD_PRIORITY,
        viMode,
        1
    );
}

OSSched* rspSchedulerGet() {
    return &sScheduler;
}
