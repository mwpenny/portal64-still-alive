#include "load_game.h"

#include "audio/soundplayer.h"
#include "levels/levels.h"
#include "savefile/savefile.h"
#include "strings/translations.h"
#include "system/controller.h"
#include "util/memory.h"

#include "codegen/assets/audio/clips.h"
#include "codegen/assets/strings/strings.h"

void loadGameMenuInit(struct LoadGameMenu* loadGame, struct SavefileListMenu* savefileList) {
    loadGame->savefileList = savefileList;
}

void loadGamePopulate(struct LoadGameMenu* loadGame) {
    struct SavefileInfo savefileInfo[MAX_SAVE_SLOTS];
    struct SaveSlotInfo saveSlots[MAX_SAVE_SLOTS];

    int numberOfSaves = savefileGetAllSlotInfo(saveSlots, 1 /* includeAuto */);

    for (int i = 0; i < numberOfSaves; ++i) {
        struct SavefileInfo* entry = &savefileInfo[i];
        struct SaveSlotInfo* info = &saveSlots[i];

        entry->slotIndex = info->slotIndex;
        entry->testChamberNumber = info->testChamberNumber;
        entry->testSubjectNumber = info->testSubjectNumber;
        entry->savefileName = info->slotIndex == AUTOSAVE_SLOT ? translationsGet(GAMEUI_AUTOSAVE) : NULL;
        entry->isFree = 0;
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
    checkpointQueueLoad(savefileGetSlot(loadGame->savefileList));
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

        savefileClearSlot(selectedSave->slotIndex);
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
        if (controllerGetButtonDown(0, BUTTON_A)) {
            if (gScene.mainMenuMode) {
                loadGameInSelectedSlot(loadGame);
            } else {
                savefileListConfirmLoad(
                    loadGame->savefileList,
                    (ConfirmationDialogCallback)&loadGameConfirmLoadClosed,
                    loadGame
                );
            }
            soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);
        } else if (controllerGetButtonDown(0, BUTTON_Z)) {
            savefileListConfirmDeletion(
                loadGame->savefileList,
                (ConfirmationDialogCallback)&loadGameConfirmDeletionClosed,
                loadGame
            );
            soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);
        }
    }

    return InputCapturePass;
}

void loadGameRender(struct LoadGameMenu* loadGame, struct RenderState* renderState, struct GraphicsTask* task) {
    savefileListRender(loadGame->savefileList, renderState, task);
}
