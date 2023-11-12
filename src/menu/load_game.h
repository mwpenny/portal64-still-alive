#ifndef __MENU_LOAD_GAME_H__
#define __MENU_LOAD_GAME_H__

#include "./savefile_list.h"

struct LoadGameMenu {
    struct SavefileListMenu* savefileList;
};

void loadGameMenuInit(struct LoadGameMenu* loadGame, struct SavefileListMenu* savefileList);
void loadGamePopulate(struct LoadGameMenu* loadGame);
enum InputCapture loadGameUpdate(struct LoadGameMenu* loadGame);
void loadGameRender(struct LoadGameMenu* loadGame, struct RenderState* renderState, struct GraphicsTask* task);

#endif