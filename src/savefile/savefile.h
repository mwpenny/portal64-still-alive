#ifndef _SAVEFILE_H
#define _SAVEFILE_H

#include "./checkpoint.h"
#include "../controls/controller_actions.h"

#define SRAM_SIZE        0x8000 

#define THUMBNAIL_IMAGE_SIZE    2048

#define SAVE_SLOT_SIZE  (MAX_CHECKPOINT_SIZE + THUMBNAIL_IMAGE_SIZE)

#define SAVEFILE_HEADER 0xDEAE

// first save slot is always reserved for auto save
#define MAX_SAVE_SLOTS  ((int)(SRAM_SIZE / SAVE_SLOT_SIZE) - 1)
#define MAX_USER_SAVE_SLOTS (MAX_SAVE_SLOTS - 1)

enum SavefileFlags {
    SavefileFlagsFirstPortalGun = (1 << 0),
    SavefileFlagsSecondPortalGun = (1 << 1),
};

struct SaveHeader {
    unsigned header;
    unsigned char chapterProgress;
    unsigned char flags;
    unsigned char nextTestSubject;
};

struct ControlSaveState {
    unsigned char controllerSettings[2][ControllerActionSourceCount];
};

struct AudioSettingsSaveState {
    unsigned char soundVolume;
    unsigned char musicVolume;
};

#define NO_TEST_CHAMBER         0xFF
#define TEST_SUBJECT_AUTOSAVE   0xFF
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
};

struct SaveSlotInfo {
    unsigned char testChamber;
    unsigned char saveSlot;
};

extern struct SaveData gSaveData;

void savefileLoad();
void savefileSave();

void savefileSetFlags(enum SavefileFlags flags);
void savefileUnsetFlags(enum SavefileFlags flags);
int savefileReadFlags(enum SavefileFlags flags);

void savefileSaveGame(Checkpoint checkpoint, int testChamberIndex, int subjectNumber, int slotIndex);
int savefileListSaves(struct SaveSlotInfo* slots);
int savefileNextTestSubject();
int savefileSuggestedSlot(int testSubject);
int savefileOldestSlot();
void savefileLoadGame(int slot, Checkpoint checkpoint);

#endif