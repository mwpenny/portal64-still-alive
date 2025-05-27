
#include "savefile.h"
#include "util/memory.h"
#include "system/time.h"
#include "system/controller.h"

#include "../controls/controller_actions.h"

struct SaveData __attribute__((aligned(8))) gSaveData;
int gCurrentTestSubject = -1;

OSPiHandle gSramHandle;

extern OSMesgQueue dmaMessageQ;

#define SRAM_latency     0x5 
#define SRAM_pulse       0x0c 
#define SRAM_pageSize    0xd 
#define SRAM_relDuration 0x2

#define SRAM_CHUNK_DELAY_USECS (10 * 1000)

#define SRAM_ADDR   0x08000000

void savefileSramSave(void* dst, void* src, int size) {
    OSIoMesg dmaIoMesgBuf;

    // save checkpoint
    dmaIoMesgBuf.hdr.pri = OS_MESG_PRI_HIGH;
    dmaIoMesgBuf.hdr.retQueue = &dmaMessageQ;
    dmaIoMesgBuf.dramAddr = src;
    dmaIoMesgBuf.devAddr = (u32)dst;
    dmaIoMesgBuf.size = size;

    osWritebackDCache(src, size);
    if (osEPiStartDma(&gSramHandle, &dmaIoMesgBuf, OS_WRITE) == -1)
    {
        return;
    }
    (void) osRecvMesg(&dmaMessageQ, NULL, OS_MESG_BLOCK);

    timeUSleep(SRAM_CHUNK_DELAY_USECS);
}

int savefileSramLoad(void* sramAddr, void* ramAddr, int size) {
    OSIoMesg dmaIoMesgBuf;

    dmaIoMesgBuf.hdr.pri = OS_MESG_PRI_HIGH;
    dmaIoMesgBuf.hdr.retQueue = &dmaMessageQ;
    dmaIoMesgBuf.dramAddr = ramAddr;
    dmaIoMesgBuf.devAddr = (u32)sramAddr;
    dmaIoMesgBuf.size = size;

    osInvalDCache(ramAddr, size);
    if (osEPiStartDma(&gSramHandle, &dmaIoMesgBuf, OS_READ) == -1)
    {
        return 0;
    }
    (void) osRecvMesg(&dmaMessageQ, NULL, OS_MESG_BLOCK);

    timeUSleep(SRAM_CHUNK_DELAY_USECS);

    return 1;
}

static void savefileUpdateSlot(int slotIndex, unsigned char testChamber, unsigned char subjectNumber, unsigned char slotOrder) {
    gSaveData.saveSlotMetadata[slotIndex].testChamber = testChamber;
    gSaveData.saveSlotMetadata[slotIndex].testSubjectNumber = subjectNumber;
    gSaveData.saveSlotMetadata[slotIndex].saveSlotOrder = slotOrder;
}

void savefileNew() {
    zeroMemory(&gSaveData, sizeof(gSaveData));
    gSaveData.header.header = SAVEFILE_HEADER;

    gSaveData.header.nextTestSubject = 0;
    gSaveData.header.flags = 0;

    for (int i = 0; i < MAX_SAVE_SLOTS; ++i) {
        savefileUpdateSlot(i, NO_TEST_CHAMBER, 0xFF, 0xFF);
    }

    controllerSetDefaultSource();
    gSaveData.controls.flags = 0;
    gSaveData.controls.flags |= ControlSavePortalFunneling;
    gSaveData.controls.sensitivity = 0x4000;
    gSaveData.controls.acceleration = 0x4000;
    gSaveData.controls.deadzone = 0x4000;
    gSaveData.controls.portalRenderDepth = 2;
    gSaveData.controls.textLanguage = 0;

    gSaveData.audio.soundVolume = 0xFFFF;
    gSaveData.audio.musicVolume = 0x8000;
    gSaveData.audio.audioLanguage = 0;

    controllerSetDeadzone(gSaveData.controls.deadzone * (1.0f / 0xFFFF) * MAX_DEADZONE);
}

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

    if (!savefileSramLoad((void*)SRAM_ADDR, &gSaveData, sizeof(gSaveData))) {
        savefileNew();
    }

    if (gSaveData.header.header != SAVEFILE_HEADER) {
        savefileNew();
    }

    controllerSetDeadzone(gSaveData.controls.deadzone * (1.0f / 0xFFFF) * MAX_DEADZONE);
}

void savefileSave() {
    savefileSramSave((void*)SRAM_ADDR, &gSaveData, sizeof(gSaveData));
}

void savefileDeleteGame(int slotIndex) {
    unsigned char prevSortOrder = gSaveData.saveSlotMetadata[slotIndex].saveSlotOrder;

    // shift existing slot sort orders
    for (int i = 0; i < MAX_SAVE_SLOTS; ++i) {
        unsigned char currSlotOrder = gSaveData.saveSlotMetadata[i].saveSlotOrder;

        if (currSlotOrder > prevSortOrder && currSlotOrder < 0xFF) {
            --gSaveData.saveSlotMetadata[i].saveSlotOrder;
        }
    }

    savefileUpdateSlot(slotIndex, NO_TEST_CHAMBER, 0xFF, 0xFF);

    savefileSave();
}

#define SAVE_SLOT_SRAM_ADDRESS(index) (SRAM_ADDR + (1 + (index)) * SAVE_SLOT_SIZE)

void savefileSaveGame(Checkpoint checkpoint, u16* screenshot, int testChamberDisplayNumber, int subjectNumber, int slotIndex) {
    savefileSramSave((void*)SAVE_SLOT_SRAM_ADDRESS(slotIndex), checkpoint, MAX_CHECKPOINT_SIZE);
    savefileSramSave((void*)SCREEN_SHOT_SRAM(slotIndex), screenshot, THUMBNAIL_IMAGE_SIZE);

    unsigned char prevSortOrder = gSaveData.saveSlotMetadata[slotIndex].saveSlotOrder;

    // shift existing slot sort orders
    for (int i = 0; i < MAX_SAVE_SLOTS; ++i) {
        if (gSaveData.saveSlotMetadata[i].saveSlotOrder < prevSortOrder) {
            ++gSaveData.saveSlotMetadata[i].saveSlotOrder;
        }
    }

    savefileUpdateSlot(slotIndex, testChamberDisplayNumber, subjectNumber, 0);

    savefileSave();
}

struct SlotAndOrder {
    unsigned char saveSlot;
    unsigned char sortOrder;
};

void savefileMetadataSort(struct SlotAndOrder* result, struct SlotAndOrder* tmp, int start, int end) {
    if (start + 1 >= end) {
        return;
    }

    int mid = (start + end) >> 1;

    savefileMetadataSort(result, tmp, start, mid);
    savefileMetadataSort(result, tmp, mid, end);

    int currentOut = start;
    int aRead = start;
    int bRead = mid;

    while (aRead < mid || bRead < end) {
        if (bRead == end || (aRead < mid && result[aRead].sortOrder < result[bRead].sortOrder)) {
            tmp[currentOut] = result[aRead];
            ++currentOut;
            ++aRead;
        } else {
            tmp[currentOut] = result[bRead];
            ++currentOut;
            ++bRead;
        }
    }

    for (int i = start; i < end; ++i) {
        result[i] = tmp[i];
    }
}

int savefileListSaves(struct SaveSlotInfo* slots, int includeAuto) {
    int result = 0;

    struct SlotAndOrder unsortedResult[MAX_SAVE_SLOTS];

    for (int i = 0; i < MAX_SAVE_SLOTS; ++i) {
        if (gSaveData.saveSlotMetadata[i].testChamber == NO_TEST_CHAMBER) {
            continue;
        }

        if (i == 0 && !includeAuto) {
            continue;
        }

        unsortedResult[result].sortOrder = gSaveData.saveSlotMetadata[i].saveSlotOrder;
        unsortedResult[result].saveSlot = i;
        ++result;
    }

    struct SlotAndOrder tmp[MAX_SAVE_SLOTS];

    savefileMetadataSort(unsortedResult, tmp, 0, result);

    for (int i = 0; i < result; ++i) {
        slots[i].saveSlot = unsortedResult[i].saveSlot;
        slots[i].testChamber = gSaveData.saveSlotMetadata[unsortedResult[i].saveSlot].testChamber;
    }

    return result;
}

int savefileNextTestSubject() {
    int needsToCheck = 1;

    while (needsToCheck) {
        needsToCheck = 0;

        for (int i = 0; i < MAX_SAVE_SLOTS; ++i) {
            if (gSaveData.saveSlotMetadata[i].testSubjectNumber == gSaveData.header.nextTestSubject) {
                needsToCheck = 1;
                ++gSaveData.header.nextTestSubject;

                if (gSaveData.header.nextTestSubject > TEST_SUBJECT_MAX) {
                    gSaveData.header.nextTestSubject = 0;
                }

                break;
            }
        }
    }

    return gSaveData.header.nextTestSubject;
}

int savefileSuggestedSlot(int testSubject) {
    int result = 0;

    // 0 indicates a new save
    for (int i = 1; i < MAX_SAVE_SLOTS; ++i) {
        if (gSaveData.saveSlotMetadata[i].testSubjectNumber == testSubject && 
            (result == 0 || gSaveData.saveSlotMetadata[i].saveSlotOrder < gSaveData.saveSlotMetadata[result].saveSlotOrder)) {
            result = i;
        }
    }

    return result;
}

int savefileOldestSlot() {
    int result = 1;

    // 0 indicates a new save
    for (int i = 1; i < MAX_SAVE_SLOTS; ++i) {
        if (gSaveData.saveSlotMetadata[i].saveSlotOrder > gSaveData.saveSlotMetadata[result].saveSlotOrder) {
            result = i;
        }
    }

    return result;
}

void savefileMarkChapterProgress(int levelIndex) {
    if (levelIndex > gSaveData.header.chapterProgressLevelIndex) {
        gSaveData.header.chapterProgressLevelIndex = levelIndex;
        savefileSave();
    }
}

int savefileFirstFreeSlot() {
    for (int i = 1; i < MAX_SAVE_SLOTS; ++i) {
        if (gSaveData.saveSlotMetadata[i].testChamber == NO_TEST_CHAMBER) {
            return i;
        }
    }

    return SAVEFILE_NO_SLOT;
}

void savefileLoadGame(int slot, Checkpoint checkpoint, int* testChamberIndex, int* subjectNumber) {
    savefileSramLoad((void*)SAVE_SLOT_SRAM_ADDRESS(slot), checkpoint, MAX_CHECKPOINT_SIZE);
    *testChamberIndex = gSaveData.saveSlotMetadata[slot].testChamber;
    *subjectNumber = gSaveData.saveSlotMetadata[slot].testSubjectNumber;
}

void savefileLoadScreenshot(u16* target, u16* location) {
    if ((int)location >= SRAM_START_ADDR && (int)location <= (SRAM_START_ADDR + SRAM_SIZE)) {
        savefileSramLoad(location, target, THUMBNAIL_IMAGE_SIZE);
    } else {
        memCopy(target, location, THUMBNAIL_IMAGE_SIZE);
    }
}


u16 gScreenGrabBuffer[SAVE_SLOT_IMAGE_W * SAVE_SLOT_IMAGE_H];

#define IMAGE_SCALE_FACTOR      (int)((SCREEN_WD << 16) / SAVE_SLOT_IMAGE_W)
#define SCALE_TO_SOURCE(value)  ((IMAGE_SCALE_FACTOR * (value)) >> 16)

void savefileGrabScreenshot() {
    u16* cfb = osViGetCurrentFramebuffer();
    u16* dst = gScreenGrabBuffer;

    for (int y = 0; y < SAVE_SLOT_IMAGE_H; ++y) {
        for (int x = 0; x < SAVE_SLOT_IMAGE_W; ++x) {
            int srcX = SCALE_TO_SOURCE(x);
            int srcY = SCALE_TO_SOURCE(y);

            *dst = cfb[srcX + srcY * SCREEN_WD];

            ++dst;
        }
    }
}
