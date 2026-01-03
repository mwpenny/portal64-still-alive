#include "savefile.h"

#include "controls/controller_actions.h"
#include "system/controller.h"
#include "system/screen.h"
#include "util/memory.h"

struct SaveData __attribute__((aligned(8))) gSaveData;
int gCurrentTestSubject = -1;

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
    if (!sramRead(0, &gSaveData, sizeof(gSaveData))) {
        savefileNew();
    }

    if (gSaveData.header.header != SAVEFILE_HEADER) {
        savefileNew();
    }

    controllerSetDeadzone(gSaveData.controls.deadzone * (1.0f / 0xFFFF) * MAX_DEADZONE);
}

void savefileSave() {
    sramWrite(0, &gSaveData, sizeof(gSaveData));
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

void savefileSaveGame(Checkpoint checkpoint, u16* screenshot, int testChamberDisplayNumber, int subjectNumber, int slotIndex) {
    sramWrite((void*)SAVE_SLOT_OFFSET(slotIndex), checkpoint, MAX_CHECKPOINT_SIZE);
    sramWrite((void*)SAVE_SLOT_SCREENSHOT_OFFSET(slotIndex), screenshot, THUMBNAIL_IMAGE_SIZE);

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
    sramRead((void*)SAVE_SLOT_OFFSET(slot), checkpoint, MAX_CHECKPOINT_SIZE);
    *testChamberIndex = gSaveData.saveSlotMetadata[slot].testChamber;
    *subjectNumber = gSaveData.saveSlotMetadata[slot].testSubjectNumber;
}

u16 gScreenGrabBuffer[SAVE_SLOT_IMAGE_W * SAVE_SLOT_IMAGE_H];

#define IMAGE_SCALE_FACTOR      (int)((SCREEN_WD << 16) / SAVE_SLOT_IMAGE_W)
#define SCALE_TO_SOURCE(value)  ((IMAGE_SCALE_FACTOR * (value)) >> 16)

void savefileGrabScreenshot() {
    u16* cfb = screenGetCurrentFramebuffer();
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

void savefileLoadScreenshot(u16* target, u16* location) {
    if (location != gScreenGrabBuffer) {
        sramRead(location, target, THUMBNAIL_IMAGE_SIZE);
    } else {
        memCopy(target, location, THUMBNAIL_IMAGE_SIZE);
    }
}
