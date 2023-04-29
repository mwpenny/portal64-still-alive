#ifndef _SAVEFILE_H
#define _SAVEFILE_H

#include "./checkpoint.h"
#include "../controls/controller_actions.h"

#define SRAM_SIZE        0x8000 

#define SAVEFILE_HEADER 0xDEAD

enum SavefileFlags {
    SavefileFlagsFirstPortalGun = (1 << 0),
    SavefileFlagsSecondPortalGun = (1 << 1),
};

struct LevelSaveData {
    char checkpoint[MAX_CHECKPOINT_SIZE];
};

struct SaveHeader {
    unsigned header;
    unsigned char chapterProgress;
    unsigned char flags;
    unsigned short saveSlotCount;
    unsigned short nextSaveSlot;
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
};

struct SaveSlotInfo {
    unsigned char testChamber;
    unsigned char saveSlot;
};

// first save slot is always reserved for auto save
#define MAX_SAVE_SLOTS  ((int)((SRAM_SIZE - sizeof(struct SaveData)) / sizeof(struct LevelSaveData)) - 1)

void savefileLoad();
void savefileSave();

void savefileSetFlags(enum SavefileFlags flags);
void savefileUnsetFlags(enum SavefileFlags flags);
int savefileReadFlags(enum SavefileFlags flags);

void savefileSaveGame(void* checkpoint);
int savefileListSaves(struct SaveSlotInfo* slots);
void saveFileLoadGame(int slot, void* checkpoint);

#endif