#ifndef __MENU_MAIN_MENU_H___
#define __MENU_MAIN_MENU_H___

#include "./game_menu.h"

void mainMenuInit(struct GameMenu* gameMenu);
void mainMenuUpdate(struct GameMenu* gameMenu);
void mainMenuRender(struct GameMenu* gameMenu, struct RenderState* renderState, struct GraphicsTask* task);

#endif