#ifndef __MENU_SAVE_GAME_H__
#define __MENU_SAVE_GAME_H__

#include "./savefile_list.h"

struct SaveGameMenu {
    struct SavefileListMenu* savefileList;
};

void saveGameMenuInit(struct SaveGameMenu* saveGame, struct SavefileListMenu* savefileList);
void saveGamePopulate(struct SaveGameMenu* saveGame, int includeNew);
enum MenuDirection saveGameUpdate(struct SaveGameMenu* saveGame);
void saveGameRender(struct SaveGameMenu* saveGame, struct RenderState* renderState, struct GraphicsTask* task);

#endif