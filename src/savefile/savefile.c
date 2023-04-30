
#include "savefile.h"
#include "util/memory.h"
#include "controls/controller.h"

#include "../controls/controller_actions.h"

struct SaveData gSaveData;

#ifdef DEBUG
#define UNLOCK_ALL  1
#else
#define UNLOCK_ALL  1
#endif

OSPiHandle gSramHandle;
OSMesgQueue     timerQueue;
OSMesg     timerQueueBuf;

extern OSMesgQueue dmaMessageQ;

void savefileNew() {
    zeroMemory(&gSaveData, sizeof(gSaveData));
    gSaveData.header.header = SAVEFILE_HEADER;

    gSaveData.header.nextSaveSlot = 1;

    controllerSetDefaultSource();

    gSaveData.audio.soundVolume = 0xFF;
    gSaveData.audio.musicVolume = 0xFF;
}

#define SRAM_START_ADDR  0x08000000 
#define SRAM_latency     0x5 
#define SRAM_pulse       0x0c 
#define SRAM_pageSize    0xd 
#define SRAM_relDuration 0x2

#define SRAM_CHUNK_DELAY        OS_USEC_TO_CYCLES(10 * 1000)

#define SRAM_ADDR   0x08000000

void savefileLoad() {
    /* Fill basic information */

    gSramHandle.type = 3;
    gSramHandle.baseAddress = PHYS_TO_K1(SRAM_START_ADDR);

    /* Get Domain parameters */

    gSramHandle.latency = (u8)SRAM_latency;
    gSramHandle.pulse = (u8)SRAM_pulse;
    gSramHandle.pageSize = (u8)SRAM_pageSize;
    gSramHandle.relDuration = (u8)SRAM_relDuration;
    gSramHandle.domain = PI_DOMAIN2;
    gSramHandle.speed = 0;

    osCreateMesgQueue(&timerQueue, &timerQueueBuf, 1);

    /* TODO gSramHandle.speed = */

    zeroMemory(&(gSramHandle.transferInfo), sizeof(gSramHandle.transferInfo));

    /*
    * Put the gSramHandle onto PiTable
    */

    OSIntMask saveMask = osGetIntMask();
    osSetIntMask(OS_IM_NONE);
    gSramHandle.next = __osPiTable;
    __osPiTable = &gSramHandle;
    osSetIntMask(saveMask);


    OSTimer timer;

    OSIoMesg dmaIoMesgBuf;

    dmaIoMesgBuf.hdr.pri = OS_MESG_PRI_HIGH;
    dmaIoMesgBuf.hdr.retQueue = &dmaMessageQ;
    dmaIoMesgBuf.dramAddr = &gSaveData;
    dmaIoMesgBuf.devAddr = SRAM_ADDR;
    dmaIoMesgBuf.size = sizeof(gSaveData);

    osInvalDCache(&gSaveData, sizeof(gSaveData));
    if (osEPiStartDma(&gSramHandle, &dmaIoMesgBuf, OS_READ) == -1)
    {
        savefileNew();
        return;
    }
    (void) osRecvMesg(&dmaMessageQ, NULL, OS_MESG_BLOCK);

    osSetTimer(&timer, SRAM_CHUNK_DELAY, 0, &timerQueue, 0);
    (void) osRecvMesg(&timerQueue, NULL, OS_MESG_BLOCK);

    if (gSaveData.header.header != SAVEFILE_HEADER) {
        savefileNew();
    }
}

void savefileSave() {
    OSTimer timer;

    OSIoMesg dmaIoMesgBuf;

    dmaIoMesgBuf.hdr.pri = OS_MESG_PRI_HIGH;
    dmaIoMesgBuf.hdr.retQueue = &dmaMessageQ;
    dmaIoMesgBuf.dramAddr = &gSaveData;
    dmaIoMesgBuf.devAddr = SRAM_ADDR;
    dmaIoMesgBuf.size = sizeof(gSaveData);

    osWritebackDCache(&gSaveData, sizeof(gSaveData));
    if (osEPiStartDma(&gSramHandle, &dmaIoMesgBuf, OS_WRITE) == -1)
    {
        return;
    }
    (void) osRecvMesg(&dmaMessageQ, NULL, OS_MESG_BLOCK);

    osSetTimer(&timer, SRAM_CHUNK_DELAY, 0, &timerQueue, 0);
    (void) osRecvMesg(&timerQueue, NULL, OS_MESG_BLOCK);
}

void savefileSetFlags(enum SavefileFlags flags) {
    gSaveData.header.flags |= flags;
}

void savefileUnsetFlags(enum SavefileFlags flags) {
    gSaveData.header.flags &= ~flags;
}

int savefileReadFlags(enum SavefileFlags flags) {
    return gSaveData.header.flags & flags;
}

#define SAVE_SLOT_SRAM_ADDRESS(index) (SRAM_ADDR + (1 + (index)) * SAVE_SLOT_SIZE)

void savefileSaveGame(Checkpoint checkpoint, int testChamberIndex, int isAutosave) {
    int slotIndex = 0;

    if (!isAutosave) {
        slotIndex = gSaveData.header.nextSaveSlot;
    }

    OSTimer timer;

    OSIoMesg dmaIoMesgBuf;

    dmaIoMesgBuf.hdr.pri = OS_MESG_PRI_HIGH;
    dmaIoMesgBuf.hdr.retQueue = &dmaMessageQ;
    dmaIoMesgBuf.dramAddr = checkpoint;
    dmaIoMesgBuf.devAddr = SAVE_SLOT_SRAM_ADDRESS(slotIndex);
    dmaIoMesgBuf.size = MAX_CHECKPOINT_SIZE;

    osWritebackDCache(&gSaveData, MAX_CHECKPOINT_SIZE);
    if (osEPiStartDma(&gSramHandle, &dmaIoMesgBuf, OS_WRITE) == -1)
    {
        return;
    }
    (void) osRecvMesg(&dmaMessageQ, NULL, OS_MESG_BLOCK);

    osSetTimer(&timer, SRAM_CHUNK_DELAY, 0, &timerQueue, 0);
    (void) osRecvMesg(&timerQueue, NULL, OS_MESG_BLOCK);

    gSaveData.saveSlotTestChamber[slotIndex] = testChamberIndex;

    if (isAutosave) {
        if (!(gSaveData.header.flags & SavefileFlagsHasAutosave)) {
            gSaveData.header.flags |= SavefileFlagsHasAutosave;
        }
    } else {
        gSaveData.header.nextSaveSlot = slotIndex + 1;

        if (gSaveData.header.nextSaveSlot >= MAX_SAVE_SLOTS) {
            gSaveData.header.nextSaveSlot = 1;
        }

        ++gSaveData.header.saveSlotCount;

        if (gSaveData.header.saveSlotCount >= MAX_USER_SAVE_SLOTS) {
            gSaveData.header.saveSlotCount = MAX_USER_SAVE_SLOTS;
        }
    }

    savefileSave();
}

int savefileListSaves(struct SaveSlotInfo* slots) {
    int result = 0;

    if (gSaveData.header.flags & SavefileFlagsHasAutosave) {
        slots[result].saveSlot = 0;
        slots[result].testChamber = gSaveData.saveSlotTestChamber[0];
        ++result;
    }

    int count = gSaveData.header.saveSlotCount;
    int nextSaveSlot = gSaveData.header.nextSaveSlot;

    while (count > 0) {
        nextSaveSlot = nextSaveSlot - 1;

        // 0 slot is reserved for autosave
        if (nextSaveSlot == 0) {
            nextSaveSlot = MAX_SAVE_SLOTS - 1;
        }

        slots[result].saveSlot = nextSaveSlot;
        slots[result].testChamber = gSaveData.saveSlotTestChamber[nextSaveSlot];

        --count;
    }

    return result;
}

void savefileLoadGame(int slot, Checkpoint checkpoint) {
    OSTimer timer;

    OSIoMesg dmaIoMesgBuf;

    dmaIoMesgBuf.hdr.pri = OS_MESG_PRI_HIGH;
    dmaIoMesgBuf.hdr.retQueue = &dmaMessageQ;
    dmaIoMesgBuf.dramAddr = checkpoint;
    dmaIoMesgBuf.devAddr = SAVE_SLOT_SRAM_ADDRESS(slot);
    dmaIoMesgBuf.size = MAX_CHECKPOINT_SIZE;

    osInvalDCache(checkpoint, MAX_CHECKPOINT_SIZE);
    if (osEPiStartDma(&gSramHandle, &dmaIoMesgBuf, OS_READ) == -1)
    {
        savefileNew();
        return;
    }
    (void) osRecvMesg(&dmaMessageQ, NULL, OS_MESG_BLOCK);

    osSetTimer(&timer, SRAM_CHUNK_DELAY, 0, &timerQueue, 0);
    (void) osRecvMesg(&timerQueue, NULL, OS_MESG_BLOCK);
}