#ifndef __MENU_MAIN_MENU_H___
#define __MENU_MAIN_MENU_H___

#include "../graphics/graphics.h"

struct MainMenu {
    Gfx* newGameText;
    Gfx* loadGameText;
    Gfx* optionsText;
};

void mainMenuInit(struct MainMenu* mainMenu);
void mainMenuUpdate(struct MainMenu* mainMenu);
void mainMenuRender(struct MainMenu* mainMenu, struct RenderState* renderState, struct GraphicsTask* task);

#endif