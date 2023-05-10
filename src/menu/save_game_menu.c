#include "./save_game_menu.h"

#include "../savefile/savefile.h"
#include "../levels/levels.h"
#include "../controls/controller.h"
#include "../util/memory.h"
#include "../audio/soundplayer.h"

#include "../build/src/audio/clips.h"

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
        savefileInfo[i].testchamberIndex = saveSlots[i].testChamber;
        savefileInfo[i].savefileName = NULL;
        savefileInfo[i].screenshot = (u16*)SCREEN_SHOT_SRAM(saveSlots[i].saveSlot);

        if (suggestedSlot == saveSlots[i].saveSlot) {
            startSelection = i;
        }
    }

    int freeSlot = savefileFirstFreeSlot();

    if (includeNew && freeSlot != SAVEFILE_NO_SLOT) {
        savefileInfo[numberOfSaves].slotIndex = freeSlot;
        savefileInfo[numberOfSaves].savefileName = "NEW SAVE";
        savefileInfo[numberOfSaves].testchamberIndex = gCurrentLevelIndex;
        savefileInfo[numberOfSaves].screenshot = gScreenGrabBuffer;

        if (suggestedSlot == 0) {
            startSelection = numberOfSaves; 
        }
        
        ++numberOfSaves;
    }

    savefileUseList(saveGame->savefileList, "SAVE GAME", savefileInfo, numberOfSaves);

    if (startSelection == -1) {
        saveGame->savefileList->selectedSave = numberOfSaves - 1;
    } else {
        saveGame->savefileList->selectedSave = startSelection;
    }
}

enum MenuDirection saveGameUpdate(struct SaveGameMenu* saveGame) {
    if (controllerGetButtonDown(0, A_BUTTON) && saveGame->savefileList->numberOfSaves) {
        Checkpoint* save = stackMalloc(MAX_CHECKPOINT_SIZE);
        if (checkpointSaveInto(&gScene, save)) {
            savefileSaveGame(save, gScreenGrabBuffer, gCurrentLevelIndex, gCurrentTestSubject, savefileGetSlot(saveGame->savefileList));
            saveGamePopulate(saveGame, 0);
        }
        stackMallocFree(save);
        soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL);
    }

    return savefileListUpdate(saveGame->savefileList);
}

void saveGameRender(struct SaveGameMenu* saveGame, struct RenderState* renderState, struct GraphicsTask* task) {
    savefileListRender(saveGame->savefileList, renderState, task);
}
