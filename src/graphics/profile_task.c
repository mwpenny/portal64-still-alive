#include "profile_task.h"

#include "../defs.h"


#ifdef PORTAL64_WITH_DEBUGGER
#include "../debugger/serial.h"
#endif

#include <string.h>
#include "../graphics/graphics.h"
#include "../util/memory.h"

extern u16 __attribute__((aligned(64))) zbuffer[SCREEN_HT * SCREEN_WD];

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

#define MOVE_WORD_IDX(gfx)  _SHIFTR((gfx)->words.w0, 16, 8)
#define MOVE_WORD_OFS(gfx)  _SHIFTR((gfx)->words.w0, 0, 16)
#define MOVE_WORD_DATA(gfx) ((gfx)->words.w1)

void printDisplayList(Gfx* dl, int depth, int* segments) {
    #ifdef PORTAL64_WITH_DEBUGGER
    if (depth > 20) {
        return;
    }

    for (int i = 0; i < 1000; ++i) {
        char message[64];
        int messageLen = sprintf(
            message, 
            "dl d %d 0x%08x%08x", 
            depth,
            dl->words.w0,
            dl->words.w1
        );
        gdbSendMessage(GDBDataTypeText, message, messageLen);

        int command = _SHIFTR(dl->words.w0, 24, 8);

        if (command == G_DL) {
            int address = dl->words.w1;
            int segment = _SHIFTR(address, 24, 4);
            printDisplayList((Gfx*)PHYS_TO_K0((segments[segment] + (address & 0xFFFFFF))), depth + 1, segments);
        }

        if (command == G_MOVEWORD) {
            int index = MOVE_WORD_IDX(dl);
            int offset = MOVE_WORD_OFS(dl);
            int data = MOVE_WORD_DATA(dl);

            if (index == G_MW_SEGMENT) {
                segments[(offset >> 2) & 0xF] = data;
            }
        }

        if (command == G_ENDDL) {
            return;
        }

        ++dl;
    }
    #endif
}

void profileTask(OSSched* scheduler, OSThread* currentThread, OSTask* task, u16* framebuffer) {
    // block scheduler thread
    osSetThreadPri(currentThread, RSP_PROFILE_PRIORITY);

    int segments[16];

    for (int i = 0; i < 16; ++i) {
        segments[i] = 0;
    }
    
    printDisplayList((Gfx*)task->t.data_ptr, 0, segments);

    // wait for DP to be available
    while (IO_READ(DPC_STATUS_REG) & (DPC_STATUS_DMA_BUSY | DPC_STATUS_END_VALID | DPC_STATUS_START_VALID));

    zeroMemory(framebuffer, sizeof(u16) * SCREEN_WD * SCREEN_HT);

    OSMesgQueue messageQueue;
    OSMesg messages[4];

    osCreateMesgQueue(&messageQueue, messages, 4);

    // take over event queues
    osSetEventMesg(OS_EVENT_SP, &messageQueue, (OSMesg)RSP_DONE_MSG);
    osSetEventMesg(OS_EVENT_DP, &messageQueue, (OSMesg)RDP_DONE_MSG);   
    osViSetEvent(&messageQueue, (OSMesg)VIDEO_MSG, 1);

    Gfx* curr = (Gfx*)task->t.data_ptr;

    Gfx* end = curr;

    while (_SHIFTR(end[1].words.w0, 24, 8) != G_RDPFULLSYNC) {
        ++end;
    }

#ifdef PORTAL64_WITH_DEBUGGER
    int total = end - curr;
#endif

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

#ifdef PORTAL64_WITH_DEBUGGER
            OSTime start = osGetTime();
#endif
            osSpTaskStart(task);
            OSMesg recv;

            do {
                (void)osRecvMesg(&messageQueue, &recv, OS_MESG_BLOCK);
            } while ((int)recv != RDP_DONE_MSG);

#ifdef PORTAL64_WITH_DEBUGGER
            OSTime result = osGetTime() - start;

            u64 us = OS_CYCLES_TO_NSEC(result);
#endif

            // wait for DP to be available
            while (IO_READ(DPC_STATUS_REG) & (DPC_STATUS_DMA_BUSY | DPC_STATUS_END_VALID | DPC_STATUS_START_VALID));

            copyGfx(tmp, curr, 3);

            osWritebackDCacheAll();
            
#ifdef PORTAL64_WITH_DEBUGGER
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
#endif
        }

        // char message[32];
        // sprintf(message, "step_%d", curr - (Gfx*)task->t.data_ptr);
        // gdbSendImage(message, SCREEN_WD, SCREEN_HT, G_IM_FMT_RGBA, G_IM_SIZ_16b, framebuffer);

        // sprintf(message, "step_zb_%d", curr - (Gfx*)task->t.data_ptr);
        // gdbSendImage(message, SCREEN_WD, SCREEN_HT, G_IM_FMT_RGBA, G_IM_SIZ_16b, zbuffer);

        ++curr;
    }

    
    osSetEventMesg(OS_EVENT_SP, &scheduler->interruptQ, (OSMesg)RSP_DONE_MSG);
    osSetEventMesg(OS_EVENT_DP, &scheduler->interruptQ, (OSMesg)RDP_DONE_MSG);   
    osViSetEvent(&scheduler->interruptQ, (OSMesg)VIDEO_MSG, 1);
    osSetThreadPri(currentThread, GAME_PRIORITY);
}

void profileMapAddress(void* ramAddress, const char* name) {
#ifdef PORTAL64_WITH_DEBUGGER
    char message[64];
    int messageLen = sprintf(
        message, 
        "addr 0x%08x -> %s", 
        (int)ramAddress, 
        name
    );
    gdbSendMessage(GDBDataTypeText, message, messageLen);
#endif
}

void profileClearAddressMap() {
#ifdef PORTAL64_WITH_DEBUGGER
    char message[64];
    int messageLen = sprintf(
        message, 
        "addr clearall"
    );
    gdbSendMessage(GDBDataTypeText, message, messageLen);
#endif
}