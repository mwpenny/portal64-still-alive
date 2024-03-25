#ifndef __MENU_GAME_MENU_H___
#define __MENU_GAME_MENU_H___

#include "../graphics/graphics.h"
#include "./menu.h"

#include "./landing_menu.h"
#include "./new_game_menu.h"
#include "./save_game_menu.h"
#include "./options_menu.h"
#include "./load_game.h"
#include "./confirmation_dialog.h"

enum GameMenuState {
    GameMenuStateLanding,
    GameMenuStateResumeGame,
    GameMenuStateSaveGame,
    GameMenuStateLoadGame,
    GameMenuStateNewGame,
    GameMenuStateOptions,
    GameMenuStateConfirmQuit,
    GameMenuStateQuit,
};

struct GameMenu {
    enum GameMenuState state;
    struct SavefileListMenu savefileList;
    struct LandingMenu landingMenu;
    struct NewGameMenu newGameMenu;
    struct LoadGameMenu loadGameMenu;
    struct SaveGameMenu saveGameMenu;
    struct OptionsMenu optionsMenu;
    struct ConfirmationDialog confirmationDialog;
    short currentRenderedLanguage;
};

void gameMenuInit(struct GameMenu* gameMenu, struct LandingMenuOption* options, int optionCount, int darkenBackground);
void gameMenuRebuildText(struct GameMenu* gameMenu);
void gameMenuUpdate(struct GameMenu* gameMenu);
void gameMenuRender(struct GameMenu* gameMenu, struct RenderState* renderState, struct GraphicsTask* task);

#endif