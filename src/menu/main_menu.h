#ifndef __MENU_MAIN_MENU_H___
#define __MENU_MAIN_MENU_H___

#include "../graphics/graphics.h"
#include "./menu.h"
#include "./menu_state.h"

#include "./landing_menu.h"
#include "./new_game_menu.h"
#include "./options_menu.h"
#include "./load_game.h"

struct MainMenu {
    enum MainMenuState state;
    struct SavefileListMenu savefileList;
    struct LandingMenu landingMenu;
    struct NewGameMenu newGameMenu;
    struct LoadGameMenu loadGameMenu;
    struct OptionsMenu optionsMenu;
};

void mainMenuInit(struct MainMenu* mainMenu);
void mainMenuUpdate(struct MainMenu* mainMenu);
void mainMenuRender(struct MainMenu* mainMenu, struct RenderState* renderState, struct GraphicsTask* task);

#endif