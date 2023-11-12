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
        savefileInfo[i].testchamberIndex = saveSlots[i].testChamber;
        savefileInfo[i].savefileName = saveSlots[i].saveSlot == 0 ? translationsGet(GAMEUI_AUTOSAVE) : NULL;
        savefileInfo[i].screenshot = (u16*)SCREEN_SHOT_SRAM(saveSlots[i].saveSlot);
    }

    savefileUseList(loadGame->savefileList, translationsGet(GAMEUI_LOADGAME), savefileInfo, numberOfSaves);
}

enum InputCapture loadGameUpdate(struct LoadGameMenu* loadGame) {
    if (controllerGetButtonDown(0, A_BUTTON) && loadGame->savefileList->numberOfSaves) {
        Checkpoint* save = stackMalloc(MAX_CHECKPOINT_SIZE);
        int testChamber;
        int testSubject;
        savefileLoadGame(savefileGetSlot(loadGame->savefileList), save, &testChamber, &testSubject);
        gCurrentTestSubject = testSubject;
        
        levelQueueLoad(chamberNumberGetLevel(testChamber), NULL, NULL);
        checkpointUse(save);

        stackMallocFree(save);

        soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
    }

    return savefileListUpdate(loadGame->savefileList);
}

void loadGameRender(struct LoadGameMenu* loadGame, struct RenderState* renderState, struct GraphicsTask* task) {
    savefileListRender(loadGame->savefileList, renderState, task);
}
