
#include "savefile.h"
#include "util/memory.h"
#include "controls/controller.h"

#include "../controls/controller_actions.h"

struct SaveData gSaveData;
int gShouldSave = 0;

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

    controllerSetDefaultSource();
    
    for (int controller = 0; controller < 2; ++controller) {
        for (int source = 0; source < ControllerActionSourceCount; ++source) {
            gSaveData.controls.controllerSettings[controller][source] = controllerGetSource(source, controller);
        }
    }

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

// void saveFileLoad() {
//     /* Fill basic information */

//     gSramHandle.type = 3;
//     gSramHandle.baseAddress = PHYS_TO_K1(SRAM_START_ADDR);

//     /* Get Domain parameters */

//     gSramHandle.latency = (u8)SRAM_latency;
//     gSramHandle.pulse = (u8)SRAM_pulse;
//     gSramHandle.pageSize = (u8)SRAM_pageSize;
//     gSramHandle.relDuration = (u8)SRAM_relDuration;
//     gSramHandle.domain = PI_DOMAIN2;
//     gSramHandle.speed = 0;

//     osCreateMesgQueue(&timerQueue, &timerQueueBuf, 1);

//     /* TODO gSramHandle.speed = */

//     zeroMemory(&(gSramHandle.transferInfo), sizeof(gSramHandle.transferInfo));

//     /*
//     * Put the gSramHandle onto PiTable
//     */

//     OSIntMask saveMask = osGetIntMask();
//     osSetIntMask(OS_IM_NONE);
//     gSramHandle.next = __osPiTable;
//     __osPiTable = &gSramHandle;
//     osSetIntMask(saveMask);


//     OSTimer timer;

//     OSIoMesg dmaIoMesgBuf;

//     dmaIoMesgBuf.hdr.pri = OS_MESG_PRI_HIGH;
//     dmaIoMesgBuf.hdr.retQueue = &dmaMessageQ;
//     dmaIoMesgBuf.dramAddr = &gSaveData;
//     dmaIoMesgBuf.devAddr = SRAM_ADDR;
//     dmaIoMesgBuf.size = sizeof(gSaveData);

//     osInvalDCache(&gSaveData, sizeof(gSaveData));
//     if (osEPiStartDma(&gSramHandle, &dmaIoMesgBuf, OS_READ) == -1)
//     {
//         saveFileNew();
//         return;
//     }
//     (void) osRecvMesg(&dmaMessageQ, NULL, OS_MESG_BLOCK);

//     osSetTimer(&timer, SRAM_CHUNK_DELAY, 0, &timerQueue, 0);
//     (void) osRecvMesg(&timerQueue, NULL, OS_MESG_BLOCK);

//     if (gSaveData.header != SAVEFILE_HEADER) {
//         saveFileNew();
//     }
// }

// void saveFileCheckSave() {
//     if (gShouldSave) {
//         OSTimer timer;

//         OSIoMesg dmaIoMesgBuf;

//         dmaIoMesgBuf.hdr.pri = OS_MESG_PRI_HIGH;
//         dmaIoMesgBuf.hdr.retQueue = &dmaMessageQ;
//         dmaIoMesgBuf.dramAddr = &gSaveData;
//         dmaIoMesgBuf.devAddr = SRAM_ADDR;
//         dmaIoMesgBuf.size = sizeof(gSaveData);

//         osWritebackDCache(&gSaveData, sizeof(gSaveData));
//         if (osEPiStartDma(&gSramHandle, &dmaIoMesgBuf, OS_WRITE) == -1)
//         {
//             gShouldSave = 0;
//             return;
//         }
//         (void) osRecvMesg(&dmaMessageQ, NULL, OS_MESG_BLOCK);

//         osSetTimer(&timer, SRAM_CHUNK_DELAY, 0, &timerQueue, 0);
//         (void) osRecvMesg(&timerQueue, NULL, OS_MESG_BLOCK);
        
//         gShouldSave = 0;
//     } else {
//         gShouldSave = 0;
//     }
// }

// void saveFileSave() {
//     gShouldSave = 1;
// }

// int saveFileIsLevelComplete(int level) {
//     return UNLOCK_ALL || gSaveData.levels[level].completionTime != 0;
// }

// unsigned short saveFileLevelTime(int level) {
//     return gSaveData.levels[level].completionTime;
// }

// void saveFileMarkLevelComplete(int level, float time) {
//     unsigned short newTime = (unsigned short)(time * 10.0f);
//     if (gSaveData.levels[level].completionTime == 0 || newTime < gSaveData.levels[level].completionTime) {
//         gSaveData.levels[level].completionTime = newTime;
//     }
//     gShouldSave = 1;
// }

// void saveFileErase() {
//     saveFileNew();
//     gShouldSave = 1;
// }


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

void savefileSetFlags(enum SavefileFlags flags) {
    gSaveData.header.flags |= flags;
}

void savefileUnsetFlags(enum SavefileFlags flags) {
    gSaveData.header.flags &= ~flags;
}

int savefileReadFlags(enum SavefileFlags flags) {
    return gSaveData.header.flags & flags;
}

void savefileSave();

void savefileSaveGame(void* checkpoint);
int savefileListSaves(struct SaveSlotInfo* slots);
void saveFileLoadGame(int slot, void* checkpoint);