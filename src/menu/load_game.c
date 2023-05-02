#include "load_game.h"

#include "../savefile/savefile.h"

void loadGameMenuInit(struct LoadGameMenu* loadGame, struct SavefileListMenu* savefileList) {
    loadGame->savefileList = savefileList;
}

void loadGamePopulate(struct LoadGameMenu* loadGame) {
    struct SavefileInfo savefileInfo[MAX_SAVE_SLOTS];
    struct SaveSlotInfo saveSlots[MAX_SAVE_SLOTS];

    int numberOfSaves = savefileListSaves(saveSlots, 1);

    saveSlots[0].saveSlot = 0;
    saveSlots[0].testChamber = 0;

    saveSlots[1].saveSlot = 1;
    saveSlots[1].testChamber = 4;

    saveSlots[2].saveSlot = 2;
    saveSlots[2].testChamber = 5;

    saveSlots[3].saveSlot = 3;
    saveSlots[3].testChamber = 3;

    saveSlots[4].saveSlot = 4;
    saveSlots[4].testChamber = 0;
    numberOfSaves = 5;

    for (int i = 0; i < numberOfSaves; ++i) {
        savefileInfo[i].slotIndex = saveSlots[i].saveSlot;
        savefileInfo[i].testchamberIndex = saveSlots[i].testChamber;
        savefileInfo[i].savefileName = NULL;
    }

    savefileUseList(loadGame->savefileList, "LOAD GAME", savefileInfo, numberOfSaves);
}

enum MenuDirection loadGameUpdate(struct LoadGameMenu* loadGame) {
    return savefileListUpdate(loadGame->savefileList);
}

void loadGameRender(struct LoadGameMenu* loadGame, struct RenderState* renderState, struct GraphicsTask* task) {
    savefileListRender(loadGame->savefileList, renderState, task);
}
