#include "./save_game_menu.h"

#include "../savefile/savefile.h"
#include "../levels/levels.h"

void saveGameMenuInit(struct SaveGameMenu* saveGame, struct SavefileListMenu* savefileList) {
    saveGame->savefileList = savefileList;
}

void saveGamePopulate(struct SaveGameMenu* saveGame) {
    struct SavefileInfo savefileInfo[MAX_SAVE_SLOTS];
    struct SaveSlotInfo saveSlots[MAX_SAVE_SLOTS];

    int numberOfSaves = savefileListSaves(saveSlots, 0);

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

    if (numberOfSaves < MAX_USER_SAVE_SLOTS) {
        // TODO
        savefileInfo[numberOfSaves].slotIndex = 0;
        savefileInfo[numberOfSaves].savefileName = "NEW SAVE";
        savefileInfo[numberOfSaves].testchamberIndex = gCurrentLevelIndex;
        ++numberOfSaves;
    }

    savefileUseList(saveGame->savefileList, "SAVE GAME", savefileInfo, numberOfSaves);

    saveGame->savefileList->selectedSave = savefileSuggestedSlot(gCurrentTestSubject);
}

enum MenuDirection saveGameUpdate(struct SaveGameMenu* saveGame) {
    return savefileListUpdate(saveGame->savefileList);
}

void saveGameRender(struct SaveGameMenu* saveGame, struct RenderState* renderState, struct GraphicsTask* task) {
    savefileListRender(saveGame->savefileList, renderState, task);
}
