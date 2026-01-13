#include "save_game_menu.h"

#include "audio/soundplayer.h"
#include "levels/levels.h"
#include "savefile/savefile.h"
#include "strings/translations.h"
#include "system/controller.h"
#include "util/memory.h"

#include "codegen/assets/audio/clips.h"
#include "codegen/assets/strings/strings.h"

void saveGameMenuInit(struct SaveGameMenu* saveGame, struct SavefileListMenu* savefileList) {
    saveGame->savefileList = savefileList;
}

void saveGamePopulate(struct SaveGameMenu* saveGame, int includeNew) {
    struct SavefileInfo savefileInfo[MAX_SAVE_SLOTS];
    struct SaveSlotInfo saveSlots[MAX_SAVE_SLOTS];

    int numberOfSaves = savefileGetAllSlotInfo(saveSlots, 0 /* includeAuto */);

    int subjectSlot = savefileLatestSubjectSlot(gCurrentTestSubject, 0 /* includeAuto */);
    int startSelection = -1;

    for (int i = 0; i < numberOfSaves; ++i) {
        struct SavefileInfo* entry = &savefileInfo[i];
        struct SaveSlotInfo* info = &saveSlots[i];

        entry->slotIndex = info->slotIndex;
        entry->testChamberNumber = info->testChamberNumber;
        entry->testSubjectNumber = info->testSubjectNumber;
        entry->savefileName = NULL;
        entry->isFree = 0;

        if (subjectSlot == info->slotIndex) {
            startSelection = i;
        }
    }

    int freeSlot = savefileFirstFreeSlot();

    if (includeNew && freeSlot != SAVEFILE_NO_SLOT) {
        struct SavefileInfo* newEntry = &savefileInfo[numberOfSaves];

        newEntry->slotIndex = freeSlot;
        newEntry->savefileName = translationsGet(GAMEUI_NEWSAVEGAME);
        newEntry->testChamberNumber = getChamberIndexFromLevelIndex(gCurrentLevelIndex, gScene.player.body.currentRoom);
        newEntry->testSubjectNumber = 0;
        newEntry->isFree = 1;

        if (subjectSlot == SAVEFILE_NO_SLOT) {
            startSelection = numberOfSaves; 
        }
        
        ++numberOfSaves;
    }

    savefileUseList(
        saveGame->savefileList,
        translationsGet(GAMEUI_SAVEGAME),
        translationsGet(GAMEUI_SAVE),
        savefileInfo,
        numberOfSaves
    );

    if (startSelection == -1) {
        saveGame->savefileList->selectedSave = numberOfSaves - 1;
    } else {
        saveGame->savefileList->selectedSave = startSelection;
    }
}

static void saveGameSaveInSelectedSlot(struct SaveGameMenu* saveGame) {
    if (checkpointSave(&gScene, savefileGetSlot(saveGame->savefileList))) {
        saveGamePopulate(saveGame, 1 /* includeNew */);
        soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);
    } else {
        soundPlayerPlay(SOUNDS_WPN_DENYSELECT, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);
    }
}

static void saveGameConfirmDeletionClosed(struct SaveGameMenu* saveGame, int isConfirmed) {
    if (isConfirmed) {
        short selectedSaveIndex = saveGame->savefileList->selectedSave;
        struct SavefileInfo* selectedSave = &saveGame->savefileList->savefileInfo[selectedSaveIndex];

        savefileClearSlot(selectedSave->slotIndex);
        saveGamePopulate(saveGame, 1);

        if (selectedSaveIndex >= saveGame->savefileList->numberOfSaves) {
            --selectedSaveIndex;
        }
        saveGame->savefileList->selectedSave = selectedSaveIndex;
    }
}

static void saveGameConfirmOverwriteClosed(void* saveGame, int isConfirmed) {
    if (isConfirmed) {
        saveGameSaveInSelectedSlot(saveGame);
    }
}

enum InputCapture saveGameUpdate(struct SaveGameMenu* saveGame) {
    enum InputCapture capture = savefileListUpdate(saveGame->savefileList);
    if (capture != InputCapturePass) {
        return capture;
    }

    if (saveGame->savefileList->numberOfSaves) {
        if (controllerGetButtonsDown(0, ControllerButtonA)) {
            short selectedSaveIndex = saveGame->savefileList->selectedSave;
            struct SavefileInfo* selectedSave = &saveGame->savefileList->savefileInfo[selectedSaveIndex];

            if (selectedSave->isFree) {
                saveGameSaveInSelectedSlot(saveGame);
            } else {
                savefileListConfirmOverwrite(
                    saveGame->savefileList,
                    (ConfirmationDialogCallback)&saveGameConfirmOverwriteClosed,
                    saveGame
                );
                soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);
            }
        } else if (controllerGetButtonsDown(0, ControllerButtonZ)) {
            short selectedSaveIndex = saveGame->savefileList->selectedSave;
            struct SavefileInfo* selectedSave = &saveGame->savefileList->savefileInfo[selectedSaveIndex];

            if (!selectedSave->isFree) {
                savefileListConfirmDeletion(
                    saveGame->savefileList,
                    (ConfirmationDialogCallback)&saveGameConfirmDeletionClosed,
                    saveGame
                );
                soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);
            }
        }
    }

    return InputCapturePass;
}

void saveGameRender(struct SaveGameMenu* saveGame, struct RenderState* renderState, struct GraphicsTask* task) {
    savefileListRender(saveGame->savefileList, renderState, task);
}
