#ifndef __PROFILE_TASK_H__
#define __PROFILE_TASK_H__

#include <ultra64.h>
#include <sched.h>

void profileTask(OSSched* scheduler, OSThread* currentThread, OSTask* task);
void profileMapAddress(void* original, void* ramAddress);
void profileClearAddressMap();

#endif