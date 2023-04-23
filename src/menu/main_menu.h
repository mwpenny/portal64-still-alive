#ifndef __MENU_MAIN_MENU_H___
#define __MENU_MAIN_MENU_H___

#include "../graphics/graphics.h"
#include "./menu.h"
#include "./menu_state.h"

#include "./landing_menu.h"
#include "./new_game_menu.h"

struct MainMenu {
    enum MainMenuState state;
    struct LandingMenu landingMenu;
    struct NewGameMenu newGameMenu;
};

void mainMenuInit(struct MainMenu* mainMenu);
void mainMenuUpdate(struct MainMenu* mainMenu);
void mainMenuRender(struct MainMenu* mainMenu, struct RenderState* renderState, struct GraphicsTask* task);

#endif