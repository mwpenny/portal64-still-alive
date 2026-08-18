#ifndef __RSP_SCHEDULER_LIBULTRA_H__
#define __RSP_SCHEDULER_LIBULTRA_H__

#include <sched.h>

void rspSchedulerInit(u8 viMode);
OSSched* rspSchedulerGet();

#endif
