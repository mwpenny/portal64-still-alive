#include "profile_task.h"

#include "../defs.h"

#include "../debugger/serial.h"
#include <string.h>

#define VIDEO_MSG       666
#define RSP_DONE_MSG    667
#define RDP_DONE_MSG    668

#define SAMPLES_PER_STEP    10

void copyGfx(Gfx* from, Gfx* to, int count) {
    Gfx* max = from + count;

    while (from < max) {
        *to++ = *from++;
    }
}

OSTime profileTask(OSSched* scheduler, OSThread* currentThread, OSTask* task) {
    // block scheduler thread
    osSetThreadPri(currentThread, RSP_PROFILE_PRIORITY);

    // wait for DP to be available
    while (IO_READ(DPC_STATUS_REG) & (DPC_STATUS_DMA_BUSY | DPC_STATUS_END_VALID | DPC_STATUS_START_VALID));

    OSMesgQueue messageQueue;
    OSMesg messages[4];

    osCreateMesgQueue(&messageQueue, messages, 4);

    // take over event queues
    osSetEventMesg(OS_EVENT_SP, &messageQueue, (OSMesg)RSP_DONE_MSG);
    osSetEventMesg(OS_EVENT_DP, &messageQueue, (OSMesg)RDP_DONE_MSG);   
    osViSetEvent(&messageQueue, (OSMesg)VIDEO_MSG, 1);

    Gfx* curr = (Gfx*)task->t.data_ptr;

    Gfx* end = curr;

    while (end[1].words.w0 != _SHIFTL(G_RDPFULLSYNC, 24, 8)) {
        ++end;
    }

    int total = end - curr;

    Gfx tmp[3];

    while (curr <= end) {
        for (int sample = 0; sample < SAMPLES_PER_STEP; ++sample) {
            // wait for DP to be available
            while (IO_READ(DPC_STATUS_REG) & (DPC_STATUS_DMA_BUSY | DPC_STATUS_END_VALID | DPC_STATUS_START_VALID));

            copyGfx(curr, tmp, 3);

            Gfx* dl = curr;
            gDPPipeSync(dl++);
            gDPFullSync(dl++);
            gSPEndDisplayList(dl++);

            // not very precise, but it seems to work
            osWritebackDCacheAll();

            OSTime start = osGetTime();
            osSpTaskStart(task);
            OSMesg recv;

            do {
                (void)osRecvMesg(&messageQueue, &recv, OS_MESG_BLOCK);
            } while ((int)recv != RDP_DONE_MSG);

            OSTime result = osGetTime() - start;

            u64 us = OS_CYCLES_TO_NSEC(result);

            // wait for DP to be available
            while (IO_READ(DPC_STATUS_REG) & (DPC_STATUS_DMA_BUSY | DPC_STATUS_END_VALID | DPC_STATUS_START_VALID));

            copyGfx(tmp, curr, 3);

            osWritebackDCacheAll();
            
            char message[64];
            int messageLen = sprintf(
                message, 
                "%d/%d 0x%08x%08x ms %d.%d", 
                curr - (Gfx*)task->t.data_ptr, 
                total,
                curr->words.w0,
                curr->words.w1,
                (int)(us / 1000000), 
                (int)(us % 1000000)
            );
            gdbSendMessage(GDBDataTypeText, message, messageLen);
        }

        ++curr;
    }

    
    osSetEventMesg(OS_EVENT_SP, &scheduler->interruptQ, (OSMesg)RSP_DONE_MSG);
    osSetEventMesg(OS_EVENT_DP, &scheduler->interruptQ, (OSMesg)RDP_DONE_MSG);   
    osViSetEvent(&scheduler->interruptQ, (OSMesg)VIDEO_MSG, 1);
    osSetThreadPri(currentThread, GAME_PRIORITY);

    return 0;
}