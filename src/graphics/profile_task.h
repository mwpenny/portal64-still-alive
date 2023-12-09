#ifndef __PROFILE_TASK_H__
#define __PROFILE_TASK_H__

#include <ultra64.h>
#include <sched.h>

OSTime profileTask(OSSched* scheduler, OSThread* currentThread, OSTask* task);

#endif