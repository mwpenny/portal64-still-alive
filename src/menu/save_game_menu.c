#include "./save_game_menu.h"

#include "../savefile/savefile.h"
#include "../levels/levels.h"
#include "../controls/controller.h"
#include "../util/memory.h"
#include "../audio/soundplayer.h"
#include "./translations.h"

#include "../build/src/audio/clips.h"
#include "../build/src/audio/subtitles.h"

void saveGameMenuInit(struct SaveGameMenu* saveGame, struct SavefileListMenu* savefileList) {
    saveGame->savefileList = savefileList;
}

void saveGamePopulate(struct SaveGameMenu* saveGame, int includeNew) {
    struct SavefileInfo savefileInfo[MAX_SAVE_SLOTS];
    struct SaveSlotInfo saveSlots[MAX_SAVE_SLOTS];

    int numberOfSaves = savefileListSaves(saveSlots, 0);

    int suggestedSlot = savefileSuggestedSlot(gCurrentTestSubject);
    int startSelection = -1;

    for (int i = 0; i < numberOfSaves; ++i) {
        savefileInfo[i].slotIndex = saveSlots[i].saveSlot;
        savefileInfo[i].testchamberDisplayNumber = saveSlots[i].testChamber;
        savefileInfo[i].savefileName = NULL;
        savefileInfo[i].screenshot = (u16*)SCREEN_SHOT_SRAM(saveSlots[i].saveSlot);
        savefileInfo[i].isFree = 0;

        if (suggestedSlot == saveSlots[i].saveSlot) {
            startSelection = i;
        }
    }

    int freeSlot = savefileFirstFreeSlot();

    if (includeNew && freeSlot != SAVEFILE_NO_SLOT) {
        savefileInfo[numberOfSaves].slotIndex = freeSlot;
        savefileInfo[numberOfSaves].savefileName = translationsGet(GAMEUI_NEWSAVEGAME);
        savefileInfo[numberOfSaves].testchamberDisplayNumber = getChamberDisplayNumberFromLevelIndex(gCurrentLevelIndex, gScene.player.body.currentRoom);
        savefileInfo[numberOfSaves].screenshot = gScreenGrabBuffer;
        savefileInfo[numberOfSaves].isFree = 1;

        if (suggestedSlot == 0) {
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

enum InputCapture saveGameUpdate(struct SaveGameMenu* saveGame) {
    if (saveGame->savefileList->numberOfSaves) {
        if (controllerGetButtonDown(0, A_BUTTON)) {
            Checkpoint* save = stackMalloc(MAX_CHECKPOINT_SIZE);
            if (checkpointSaveInto(&gScene, save)) {
                savefileSaveGame(save, gScreenGrabBuffer, getChamberDisplayNumberFromLevelIndex(gCurrentLevelIndex, gScene.player.body.currentRoom), gCurrentTestSubject, savefileGetSlot(saveGame->savefileList));
                saveGamePopulate(saveGame, 1);
                soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
            } else {
                soundPlayerPlay(SOUNDS_WPN_DENYSELECT, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
            }
            stackMallocFree(save);
        } else if (controllerGetButtonDown(0, Z_TRIG)) {
            short selectedSaveIndex = saveGame->savefileList->selectedSave;
            struct SavefileInfo* selectedSave = &saveGame->savefileList->savefileInfo[selectedSaveIndex];

            if (selectedSave->isFree) {
                soundPlayerPlay(SOUNDS_WPN_DENYSELECT, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
            } else {
                savefileDeleteGame(selectedSave->slotIndex);
                saveGamePopulate(saveGame, 1);

                if (selectedSaveIndex >= saveGame->savefileList->numberOfSaves) {
                    --selectedSaveIndex;
                }
                saveGame->savefileList->selectedSave = selectedSaveIndex;

                soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
            }
        }
    }

    return savefileListUpdate(saveGame->savefileList);
}

void saveGameRender(struct SaveGameMenu* saveGame, struct RenderState* renderState, struct GraphicsTask* task) {
    savefileListRender(saveGame->savefileList, renderState, task);
}
