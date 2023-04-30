#ifndef __MENU_LOAD_GAME_H__
#define __MENU_LOAD_GAME_H__

#include "./menu.h"
#include "../graphics/graphics.h"
#include "../savefile/savefile.h"
#include "./new_game_menu.h"

struct LoadGameSlot {
    Gfx* testChamberText;
    Gfx* border;
    Gfx* gameId;
    short x, y;
    short saveSlot;
    void* imageData;
};

#define MAX_VISIBLE_SLOTS  3

struct LoadGameMenu {
    Gfx* menuOutline;
    Gfx* loadGameText;
    struct SaveSlotInfo slotInfo[MAX_SAVE_SLOTS];
    struct LoadGameSlot slots[MAX_VISIBLE_SLOTS];
    short numberOfSaves;
    short scrollOffset;
    short selectedSave;
};

void loadGameMenuInit(struct LoadGameMenu* loadGame);
enum MenuDirection loadGameUpdate(struct LoadGameMenu* loadGame);
void loadGameRender(struct LoadGameMenu* loadGame, struct RenderState* renderState, struct GraphicsTask* task);

#endif