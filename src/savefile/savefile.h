#ifndef _SAVEFILE_H
#define _SAVEFILE_H

#include "./checkpoint.h"
#include "../controls/controller_actions.h"

#define SRAM_SIZE        0x8000 

#define SAVEFILE_HEADER 0xDEAD

// first save slot is always reserved for auto save
#define MAX_SAVE_SLOTS  ((int)(SRAM_SIZE / MAX_CHECKPOINT_SIZE) - 1)
#define MAX_USER_SAVE_SLOTS (MAX_SAVE_SLOTS - 1)

enum SavefileFlags {
    SavefileFlagsFirstPortalGun = (1 << 0),
    SavefileFlagsSecondPortalGun = (1 << 1),
    SavefileFlagsHasAutosave = (1 << 2),
};

struct SaveHeader {
    unsigned header;
    unsigned char chapterProgress;
    unsigned char flags;
    unsigned char saveSlotCount;
    unsigned char nextSaveSlot;
};

struct ControlSaveState {
    unsigned char controllerSettings[2][ControllerActionSourceCount];
};

struct AudioSettingsSaveState {
    unsigned char soundVolume;
    unsigned char musicVolume;
};

struct SaveData {
    struct SaveHeader header;
    struct ControlSaveState controls;
    struct AudioSettingsSaveState audio;
    unsigned char saveSlotTestChamber[MAX_SAVE_SLOTS];
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

void savefileSaveGame(Checkpoint checkpoint, int isAutosave);
int savefileListSaves(struct SaveSlotInfo* slots);
void savefileLoadGame(int slot, Checkpoint checkpoint);

#endif