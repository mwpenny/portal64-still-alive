#ifndef _SAVEFILE_H
#define _SAVEFILE_H

#include "./checkpoint.h"
#include "../controls/controller_actions.h"

#define SRAM_START_ADDR  0x08000000 
#define SRAM_SIZE        0x8000 

#define SAVEFILE_NO_SLOT    -1

#define SAVE_SLOT_IMAGE_W   36
#define SAVE_SLOT_IMAGE_H   27

#define THUMBNAIL_IMAGE_SIZE    (SAVE_SLOT_IMAGE_W * SAVE_SLOT_IMAGE_H * sizeof(u16))

#define THUMBNAIL_IMAGE_SPACE    2048

#define SAVE_SLOT_SIZE  (MAX_CHECKPOINT_SIZE + THUMBNAIL_IMAGE_SPACE)

#define SCREEN_SHOT_SRAM(slotIndex)     (((slotIndex) + 1) * SAVE_SLOT_SIZE + MAX_CHECKPOINT_SIZE + SRAM_START_ADDR)

#define SAVEFILE_HEADER 0xDEFC

// first save slot is always reserved for auto save
#define MAX_SAVE_SLOTS  ((int)(SRAM_SIZE / SAVE_SLOT_SIZE) - 1)
#define MAX_USER_SAVE_SLOTS (MAX_SAVE_SLOTS - 1)

struct SaveHeader {
    unsigned header;
    unsigned char chapterProgressLevelIndex;
    unsigned char flags;
    unsigned char nextTestSubject;
};

enum ControlSaveFlags {
    ControlSaveFlagsInvert = (1 << 0),
    ControlSaveTankControls = (1 << 2),

    ControlSaveSubtitlesEnabled = (1 << 5),
    ControlSaveAllSubtitlesEnabled = (1 << 6),

    ControlSavePortalFunneling = (1 << 7),
    ControlSaveMoveablePortals = (1 << 8),
    ControlSaveWideScreen = (1 << 9),
};

struct ControlSaveState {
    unsigned char controllerSettings[2][ControllerActionSourceCount];
    unsigned short flags;
    unsigned short sensitivity;
    unsigned short acceleration;
    unsigned short deadzone;
    unsigned char portalRenderDepth;
    unsigned char textLanguage;
};

struct AudioSettingsSaveState {
    unsigned short soundVolume;
    unsigned short musicVolume;
    unsigned char audioLanguage;
};

#define NO_TEST_CHAMBER         0xFF
#define TEST_SUBJECT_MAX        99

struct SaveSlotMetadata {
    unsigned char testChamber;
    unsigned char testSubjectNumber;
    unsigned char saveSlotOrder;
};

struct SaveData {
    struct SaveHeader header;
    struct ControlSaveState controls;
    struct AudioSettingsSaveState audio;
    struct SaveSlotMetadata saveSlotMetadata[MAX_SAVE_SLOTS];
    // without this the dma copy into SRAM is cut short
    u64 __align;
};

struct SaveSlotInfo {
    unsigned char testChamber;
    unsigned char saveSlot;
};

extern struct SaveData gSaveData;
extern int gCurrentTestSubject;

void savefileLoad();
void savefileSave();

void savefileDeleteGame(int slotIndex);

void savefileSaveGame(Checkpoint checkpoint, u16* screenshot, int testChamberIndex, int subjectNumber, int slotIndex);
int savefileListSaves(struct SaveSlotInfo* slots, int includeAuto);
int savefileNextTestSubject();
int savefileSuggestedSlot(int testSubject);
int savefileOldestSlot();

void savefileMarkChapterProgress(int chamberNumber);

int savefileFirstFreeSlot();

void savefileLoadGame(int slot, Checkpoint checkpoint, int* testChamberIndex, int* subjectNumber);
void savefileLoadScreenshot(u16* target, u16* location);

extern u16 gScreenGrabBuffer[SAVE_SLOT_IMAGE_W * SAVE_SLOT_IMAGE_H];

void savefileGrabScreenshot();

#endif
