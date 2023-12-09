#include "profile_task.h"

#include "../defs.h"

#include "../debugger/serial.h"
#include <string.h>

#define VIDEO_MSG       666
#define RSP_DONE_MSG    667
#define RDP_DONE_MSG    668

void profileGfxSwap(Gfx* a, Gfx* b, int count) {
    while (count) {
        Gfx tmp = *a;
        *a = *b;
        *b = tmp;
        ++a;
        ++b;
        --count;
    }

    osWritebackDCache(a - count, sizeof(Gfx) * count);
    osWritebackDCache(b - count, sizeof(Gfx) * count);
}

OSTime profileTask(OSSched* scheduler, OSThread* currentThread, OSTask* task) {
    osSetThreadPri(currentThread, RSP_PROFILE_PRIORITY);

    // wait for DP to be available
    while (IO_READ(DPC_STATUS_REG) & (DPC_STATUS_DMA_BUSY | DPC_STATUS_END_VALID | DPC_STATUS_START_VALID));

    OSMesgQueue messageQueue;
    OSMesg messages[4];

    osCreateMesgQueue(&messageQueue, messages, 4);

    osSetEventMesg(OS_EVENT_SP, &messageQueue, (OSMesg)RSP_DONE_MSG);
    osSetEventMesg(OS_EVENT_DP, &messageQueue, (OSMesg)RDP_DONE_MSG);   
    osViSetEvent(&messageQueue, (OSMesg)VIDEO_MSG, 1);

    Gfx* curr = (Gfx*)task->t.data_ptr;
    Gfx tmp[3];
    Gfx* dl = tmp;

    gDPPipeSync(dl++);
    gDPFullSync(dl++);
    gSPEndDisplayList(dl++);

    while (curr[1].words.w0 != _SHIFTL(G_RDPFULLSYNC, 24, 8)) {
        OSTime start = osGetTime();
        osSpTaskStart(task);
        OSMesg recv;

        profileGfxSwap(tmp, curr, 3);

        do {
            (void)osRecvMesg(&messageQueue, &recv, OS_MESG_BLOCK);
        } while ((int)recv != RDP_DONE_MSG);

        OSTime result = osGetTime() - start;

        u64 us = OS_CYCLES_TO_USEC(result);

        profileGfxSwap(tmp, curr, 3);

        char message[64];
        sprintf(
            message, 
            "0x%08x 0x%08x%08x ms %d.%d", 
            (int)curr - (int)task->t.data_ptr, 
            curr->words.w0,
            curr->words.w1,
            (int)(us / 1000), 
            (int)(us % 1000)
        );
        // gdbSendMessage(GDBDataTypeText, message, messageLen);

        ++curr;
    }

    
    osSetEventMesg(OS_EVENT_SP, &scheduler->interruptQ, (OSMesg)RSP_DONE_MSG);
    osSetEventMesg(OS_EVENT_DP, &scheduler->interruptQ, (OSMesg)RDP_DONE_MSG);   
    osViSetEvent(&scheduler->interruptQ, (OSMesg)VIDEO_MSG, 1);
    osSetThreadPri(currentThread, GAME_PRIORITY);

    return 0;
}