#include "savefile.h"

#include "controls/controller_actions.h"
#include "system/controller.h"
#include "system/screen.h"
#include "util/memory.h"

#define SAVEFILE_MAGIC                      0xDF01

#define SAVE_SLOT_OFFSET(index)             (((index) + 1) * SAVE_SLOT_SIZE)
#define SAVE_SLOT_IMAGE_OFFSET(index)       (SAVE_SLOT_OFFSET(index) + MAX_CHECKPOINT_SIZE)

#define NO_TEST_CHAMBER                     0xFF
#define TEST_SUBJECT_MAX                    99

#define SAVE_SLOT_IMAGE_SCALE_FACTOR        (int)((SCREEN_WD << 16) / SAVE_SLOT_IMAGE_W)
#define SCALE_SAVE_SLOT_IMAGE_COORD(value)  ((SAVE_SLOT_IMAGE_SCALE_FACTOR * (value)) >> 16)

struct SaveData __attribute__((aligned(8))) gSaveData;
uint8_t gCurrentTestSubject = 0;

static uint16_t sSlotImage[SAVE_SLOT_IMAGE_W * SAVE_SLOT_IMAGE_H];

static void savefileUpdateSlot(uint8_t slotIndex, uint8_t testChamber, uint8_t subjectNumber, uint8_t slotOrder) {
    struct SaveSlotMetadata* metadata = &gSaveData.saveSlotMetadata[slotIndex];

    metadata->testChamberNumber = testChamber;
    metadata->testSubjectNumber = subjectNumber;
    metadata->saveSlotOrder = slotOrder;
}

static void savefileNew() {
    zeroMemory(&gSaveData, sizeof(gSaveData));
    gSaveData.header.magic = SAVEFILE_MAGIC;

    for (int i = 0; i < MAX_SAVE_SLOTS; ++i) {
        savefileUpdateSlot(i, NO_TEST_CHAMBER, 0xFF, 0xFF);
    }

    controllerActionSetDefaultSources();
    gSaveData.controls.sensitivity = 0x4000;
    gSaveData.controls.acceleration = 0x4000;
    gSaveData.controls.deadzone = 0x4000;

    gSaveData.audio.soundVolume = 0xFFFF;
    gSaveData.audio.musicVolume = 0x8000;

    gSaveData.gameplay.portalRenderDepth = 2;
    gSaveData.gameplay.flags |= GameplaySaveFlagsPortalFunneling;
}

void savefileLoad() {
    if (!sramRead(0, &gSaveData, sizeof(gSaveData))) {
        savefileNew();
    }

    if (gSaveData.header.magic != SAVEFILE_MAGIC) {
        savefileNew();
    }

    controllerActionSetDeadzone(gSaveData.controls.deadzone * (1.0f / 0xFFFF));
}

void savefileSave() {
    sramWrite(0, &gSaveData, sizeof(gSaveData));
}

void savefileMarkChapterProgress(int testChamberNumber) {
    if (testChamberNumber > gSaveData.header.chapterProgressLevelIndex) {
        gSaveData.header.chapterProgressLevelIndex = testChamberNumber;
        savefileSave();
    }
}

int savefileLoadSlot(int slot, Checkpoint checkpoint) {
    return sramRead((void*)SAVE_SLOT_OFFSET(slot), checkpoint, MAX_CHECKPOINT_SIZE);
}

void savefileSaveSlot(int slotIndex, int testChamberNumber, int subjectNumber, Checkpoint checkpoint) {
    sramWrite((void*)SAVE_SLOT_OFFSET(slotIndex), checkpoint, MAX_CHECKPOINT_SIZE);
    sramWrite((void*)SAVE_SLOT_IMAGE_OFFSET(slotIndex), sSlotImage, SAVE_SLOT_IMAGE_SIZE);

    uint8_t prevSortOrder = gSaveData.saveSlotMetadata[slotIndex].saveSlotOrder;

    // Shift existing slot sort orders
    for (int i = 0; i < MAX_SAVE_SLOTS; ++i) {
        if (savefileSlotIsFree(i)) {
            continue;
        }

        uint8_t* currSlotOrder = &gSaveData.saveSlotMetadata[i].saveSlotOrder;
        if (*currSlotOrder < prevSortOrder) {
            ++*currSlotOrder;
        }
    }

    savefileUpdateSlot(slotIndex, testChamberNumber, subjectNumber, 0);
    savefileSave();
}

void savefileClearSlot(int slotIndex) {
    uint8_t prevSortOrder = gSaveData.saveSlotMetadata[slotIndex].saveSlotOrder;

    // Shift existing slot sort orders
    for (int i = 0; i < MAX_SAVE_SLOTS; ++i) {
        if (savefileSlotIsFree(i)) {
            continue;
        }

        uint8_t* currSlotOrder = &gSaveData.saveSlotMetadata[i].saveSlotOrder;
        if (*currSlotOrder > prevSortOrder) {
            --*currSlotOrder;
        }
    }

    savefileUpdateSlot(slotIndex, NO_TEST_CHAMBER, 0xFF, 0xFF);
    savefileSave();
}

void savefileGetSlotInfo(int slotIndex, struct SaveSlotInfo* info) {
    struct SaveSlotMetadata* metadata = &gSaveData.saveSlotMetadata[slotIndex];

    info->slotIndex = slotIndex;
    info->testChamberNumber = metadata->testChamberNumber;
    info->testSubjectNumber = metadata->testSubjectNumber;
}

struct SlotAndOrder {
    uint8_t slotIndex;
    uint8_t sortOrder;
};

static void savefileMetadataSort(struct SlotAndOrder* result, struct SlotAndOrder* tmp, int start, int end) {
    // Merge sort

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

int savefileGetAllSlotInfo(struct SaveSlotInfo* slots, int includeAuto) {
    struct SlotAndOrder result[MAX_SAVE_SLOTS];
    int slotCount = 0;

    for (int i = 0; i < MAX_SAVE_SLOTS; ++i) {
        if (savefileSlotIsFree(i)) {
            continue;
        }

        if (i == AUTOSAVE_SLOT && !includeAuto) {
            continue;
        }

        result[slotCount].slotIndex = i;
        result[slotCount].sortOrder = gSaveData.saveSlotMetadata[i].saveSlotOrder;
        ++slotCount;
    }

    struct SlotAndOrder tmp[MAX_SAVE_SLOTS];
    savefileMetadataSort(result, tmp, 0, slotCount);

    for (int i = 0; i < slotCount; ++i) {
        savefileGetSlotInfo(result[i].slotIndex, &slots[i]);
    }

    return slotCount;
}

int savefileSlotIsFree(int slotIndex) {
    return slotIndex >= 0 &&
        slotIndex < MAX_SAVE_SLOTS &&
        gSaveData.saveSlotMetadata[slotIndex].testChamberNumber == NO_TEST_CHAMBER;
}

int savefileFirstFreeSlot() {
    for (int i = 0; i < MAX_SAVE_SLOTS; ++i) {
        if (i == AUTOSAVE_SLOT) {
            continue;
        }

        if (savefileSlotIsFree(i)) {
            return i;
        }
    }

    return SAVEFILE_NO_SLOT;
}

int savefileNextTestSubject() {
    int needsToCheck = 1;

    while (needsToCheck) {
        needsToCheck = 0;

        for (int i = 0; i < MAX_SAVE_SLOTS; ++i) {
            if (savefileSlotIsFree(i)) {
                continue;
            }

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

    savefileSave();
    return gSaveData.header.nextTestSubject;
}

int savefileLatestSubjectSlot(int testSubjectNumber, int includeAuto) {
    int result = SAVEFILE_NO_SLOT;

    for (int i = 0; i < MAX_SAVE_SLOTS; ++i) {
        if (savefileSlotIsFree(i)) {
            continue;
        }

        if (i == AUTOSAVE_SLOT && !includeAuto) {
            continue;
        }

        struct SaveSlotMetadata* metadata = &gSaveData.saveSlotMetadata[i];

        if (metadata->testSubjectNumber == testSubjectNumber &&
            (result == SAVEFILE_NO_SLOT || metadata->saveSlotOrder < gSaveData.saveSlotMetadata[result].saveSlotOrder)
        ) {
            result = i;
        }
    }

    return result;
}

void savefileUpdateSlotImage() {
    uint16_t* cfb = screenGetCurrentFramebuffer();
    uint16_t* dst = sSlotImage;

    for (int y = 0; y < SAVE_SLOT_IMAGE_H; ++y) {
        for (int x = 0; x < SAVE_SLOT_IMAGE_W; ++x) {
            int srcX = SCALE_SAVE_SLOT_IMAGE_COORD(x);
            int srcY = SCALE_SAVE_SLOT_IMAGE_COORD(y);

            *dst = cfb[srcX + (srcY * SCREEN_WD)];

            ++dst;
        }
    }
}

void savefileCopySlotImage(int slotIndex, void* dest) {
    if (savefileSlotIsFree(slotIndex)) {
        // TODO: "new slot" image (not screenshot)
        memCopy(dest, sSlotImage, SAVE_SLOT_IMAGE_SIZE);
    } else {
        sramRead((void*)SAVE_SLOT_IMAGE_OFFSET(slotIndex), dest, SAVE_SLOT_IMAGE_SIZE);
    }
}
