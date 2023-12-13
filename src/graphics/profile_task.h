#ifndef __PROFILE_TASK_H__
#define __PROFILE_TASK_H__

#include <ultra64.h>
#include <sched.h>

void profileTask(OSSched* scheduler, OSThread* currentThread, OSTask* task, u16* framebuffer);
void profileMapAddress(void* ramAddress, const char* name);
void profileClearAddressMap();

#endif