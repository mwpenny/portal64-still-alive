#include "load_game.h"

#include "../savefile/savefile.h"
#include "../controls/controller.h"
#include "../levels/levels.h"
#include "../util/memory.h"
#include "../audio/soundplayer.h"
#include "./translations.h"

#include "../build/src/audio/clips.h"
#include "../build/src/audio/subtitles.h"

void loadGameMenuInit(struct LoadGameMenu* loadGame, struct SavefileListMenu* savefileList) {
    loadGame->savefileList = savefileList;
}

void loadGamePopulate(struct LoadGameMenu* loadGame) {
    struct SavefileInfo savefileInfo[MAX_SAVE_SLOTS];
    struct SaveSlotInfo saveSlots[MAX_SAVE_SLOTS];

    int numberOfSaves = savefileListSaves(saveSlots, 1);

    for (int i = 0; i < numberOfSaves; ++i) {
        savefileInfo[i].slotIndex = saveSlots[i].saveSlot;
        savefileInfo[i].testchamberDisplayNumber = saveSlots[i].testChamber;
        savefileInfo[i].savefileName = saveSlots[i].saveSlot == 0 ? translationsGet(GAMEUI_AUTOSAVE) : NULL;
        savefileInfo[i].screenshot = (u16*)SCREEN_SHOT_SRAM(saveSlots[i].saveSlot);
        savefileInfo[i].isFree = 0;
    }

    savefileUseList(
        loadGame->savefileList,
        translationsGet(GAMEUI_LOADGAME),
        translationsGet(GAMEUI_LOAD),
        savefileInfo,
        numberOfSaves
    );
}

static void loadGameInSelectedSlot(struct LoadGameMenu* loadGame) {
    Checkpoint* save = stackMalloc(MAX_CHECKPOINT_SIZE);
    int testChamber;
    int testSubject;
    savefileLoadGame(savefileGetSlot(loadGame->savefileList), save, &testChamber, &testSubject);
    gCurrentTestSubject = testSubject;

    levelQueueLoad(getLevelIndexFromChamberDisplayNumber(testChamber), NULL, NULL);
    checkpointUse(save);

    stackMallocFree(save);
}

static void loadGameConfirmLoadClosed(struct LoadGameMenu* loadGame, int isConfirmed) {
    if (isConfirmed) {
        loadGameInSelectedSlot(loadGame);
    }
}

static void loadGameConfirmDeletionClosed(struct LoadGameMenu* loadGame, int isConfirmed) {
    if (isConfirmed) {
        short selectedSaveIndex = loadGame->savefileList->selectedSave;
        struct SavefileInfo* selectedSave = &loadGame->savefileList->savefileInfo[selectedSaveIndex];

        savefileDeleteGame(selectedSave->slotIndex);
        loadGamePopulate(loadGame);

        if (selectedSaveIndex >= loadGame->savefileList->numberOfSaves) {
            --selectedSaveIndex;
        }
        loadGame->savefileList->selectedSave = selectedSaveIndex;
    }
}

enum InputCapture loadGameUpdate(struct LoadGameMenu* loadGame) {
    enum InputCapture capture = savefileListUpdate(loadGame->savefileList);
    if (capture != InputCapturePass) {
        return capture;
    }

    if (loadGame->savefileList->numberOfSaves) {
        if (controllerGetButtonDown(0, A_BUTTON)) {
            if (gScene.mainMenuMode) {
                loadGameInSelectedSlot(loadGame);
            } else {
                savefileListConfirmLoad(
                    loadGame->savefileList,
                    (ConfirmationDialogCallback)&loadGameConfirmLoadClosed,
                    loadGame
                );
            }
            soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
        } else if (controllerGetButtonDown(0, Z_TRIG)) {
            savefileListConfirmDeletion(
                loadGame->savefileList,
                (ConfirmationDialogCallback)&loadGameConfirmDeletionClosed,
                loadGame
            );
            soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
        }
    }

    return InputCapturePass;
}

void loadGameRender(struct LoadGameMenu* loadGame, struct RenderState* renderState, struct GraphicsTask* task) {
    savefileListRender(loadGame->savefileList, renderState, task);
}
