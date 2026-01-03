#ifndef __SAVEFILE_H__
#define __SAVEFILE_H__

#include <stdint.h>

#include "checkpoint.h"
#include "controls/controller_actions.h"
#include "system/cartridge.h"

#define SAVE_SLOT_IMAGE_W       36
#define SAVE_SLOT_IMAGE_H       27
#define SAVE_SLOT_IMAGE_SIZE    (SAVE_SLOT_IMAGE_W * SAVE_SLOT_IMAGE_H * sizeof(uint16_t))
#define SAVE_SLOT_IMAGE_SPACE   2048

#define SAVE_SLOT_SIZE          (MAX_CHECKPOINT_SIZE + SAVE_SLOT_IMAGE_SPACE)

// One slot's worth of space is reserved for global data
// The first checkpoint slot is used for autosave
#define MAX_SAVE_SLOTS          ((int)(SRAM_SIZE / SAVE_SLOT_SIZE) - 1)
#define AUTOSAVE_SLOT           0
#define SAVEFILE_NO_SLOT        -1

enum ControlSaveFlags {
    ControlSaveFlagsInvert = (1 << 0),
    ControlSaveFlagsTankControls = (1 << 1),
};

enum VideoSaveFlags {
    VideoSaveFlagsSubtitlesEnabled = (1 << 0),
    VideoSaveFlagsCaptionsEnabled = (1 << 1),
    VideoSaveFlagsWideScreen = (1 << 2),
};

enum GameplaySaveFlags {
    GameplaySaveFlagsPortalFunneling = (1 << 0),
    GameplaySaveFlagsMovablePortals = (1 << 1),
};

struct SaveHeader {
    uint16_t magic;
    uint8_t chapterProgressLevelIndex;
    uint8_t nextTestSubject;
};

struct ControlSaveState {
    uint8_t controllerSettings[2][ControllerActionSourceCount];
    enum ControlSaveFlags flags;
    uint16_t sensitivity;
    uint16_t acceleration;
    uint16_t deadzone;
};

struct AudioSettingsSaveState {
    uint16_t soundVolume;
    uint16_t musicVolume;
    uint8_t audioLanguage;
};

struct VideoSettingsSaveState {
    enum VideoSaveFlags flags;
    uint8_t textLanguage;
};

struct GameplaySaveState {
    enum GameplaySaveFlags flags;
    uint8_t portalRenderDepth;
};

struct SaveSlotMetadata {
    uint8_t testChamberNumber;
    uint8_t testSubjectNumber;
    uint8_t saveSlotOrder;
};

struct SaveData {
    struct SaveHeader header;
    struct ControlSaveState controls;
    struct AudioSettingsSaveState audio;
    struct VideoSettingsSaveState video;
    struct GameplaySaveState gameplay;
    struct SaveSlotMetadata saveSlotMetadata[MAX_SAVE_SLOTS];

    // Without this, the DMA copy into SRAM is cut short
    uint64_t __align;
};

struct SaveSlotInfo {
    uint8_t slotIndex;
    uint8_t testChamberNumber;
    uint8_t testSubjectNumber;
};

extern struct SaveData gSaveData;
extern uint8_t gCurrentTestSubject;

void savefileLoad();
void savefileSave();
void savefileMarkChapterProgress(int testChamberNumber);

int savefileLoadSlot(int slotIndex, Checkpoint checkpoint);
void savefileSaveSlot(int slotIndex, int testChamberNumber, int subjectNumber, Checkpoint checkpoint);
void savefileClearSlot(int slotIndex);

void savefileGetSlotInfo(int slotIndex, struct SaveSlotInfo* info);
int savefileGetAllSlotInfo(struct SaveSlotInfo* slots, int includeAuto);

int savefileSlotIsFree(int slotIndex);
int savefileFirstFreeSlot();

int savefileNextTestSubject();
int savefileLatestSubjectSlot(int testSubjectNumber, int includeAuto);

void savefileUpdateSlotImage();
void savefileCopySlotImage(int slotIndex, void* dest);

#endif
