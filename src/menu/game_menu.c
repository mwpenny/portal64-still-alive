#include "./game_menu.h"

#include "../util/memory.h"
#include "../util/rom.h"
#include "menu.h"
#include <string.h>

#include "../build/assets/materials/ui.h"
#include "../scene/render_plan.h"
#include "../controls/controller.h"
#include "./translations.h"

void gameMenuInit(struct GameMenu* gameMenu, struct LandingMenuOption* options, int optionCount, int darkenBackground) {
    landingMenuInit(&gameMenu->landingMenu, options, optionCount, darkenBackground);
    savefileListMenuInit(&gameMenu->savefileList);
    newGameInit(&gameMenu->newGameMenu);
    loadGameMenuInit(&gameMenu->loadGameMenu, &gameMenu->savefileList);
    saveGameMenuInit(&gameMenu->saveGameMenu, &gameMenu->savefileList);
    optionsMenuInit(&gameMenu->optionsMenu);

    gameMenu->state = GameMenuStateLanding;
    gameMenu->currentRenderedLanguage = translationsCurrentLanguage();
}

void gameMenuRebuildText(struct GameMenu* gameMenu) {
    if (gameMenu->currentRenderedLanguage != translationsCurrentLanguage()) {
        newGameRebuildText(&gameMenu->newGameMenu);
        landingMenuRebuildText(&gameMenu->landingMenu);
        optionsMenuRebuildText(&gameMenu->optionsMenu);
        gameMenu->currentRenderedLanguage = translationsCurrentLanguage();
    }
}

enum GameMenuState gameInputCaptureToState(enum InputCapture direction, enum GameMenuState currentState) {
    if (direction == InputCaptureExit) {
        return GameMenuStateLanding;
    }

    return currentState;
}

void gameMenuUpdate(struct GameMenu* gameMenu) {
    enum GameMenuState prevState = gameMenu->state;

    switch (gameMenu->state) {
        case GameMenuStateLanding:
        {
            struct LandingMenuOption* option = landingMenuUpdate(&gameMenu->landingMenu);
            
            if (option) {
                gameMenu->state = option->id;
            }

            break;
        }
        case GameMenuStateNewGame:
            gameMenu->state = gameInputCaptureToState(newGameUpdate(&gameMenu->newGameMenu), gameMenu->state);
            break;
        case GameMenuStateLoadGame:
            gameMenu->state = gameInputCaptureToState(loadGameUpdate(&gameMenu->loadGameMenu), gameMenu->state);
            break;
        case GameMenuStateSaveGame:
            gameMenu->state = gameInputCaptureToState(saveGameUpdate(&gameMenu->saveGameMenu), gameMenu->state);
            break;
        case GameMenuStateOptions:
            gameMenu->state = gameInputCaptureToState(optionsMenuUpdate(&gameMenu->optionsMenu), gameMenu->state);
            break;
        default:
            break;
    }

    if (prevState != gameMenu->state) {
        switch (gameMenu->state) {
            case GameMenuStateLoadGame:
                loadGamePopulate(&gameMenu->loadGameMenu);
                break;
            case GameMenuStateSaveGame:
                saveGamePopulate(&gameMenu->saveGameMenu, 1);
                break;
            default:
                break;
        }
    }
}

extern Lights1 gSceneLights;
extern LookAt gLookAt;

void gameMenuRender(struct GameMenu* gameMenu, struct RenderState* renderState, struct GraphicsTask* task) {
    switch (gameMenu->state) {
        case GameMenuStateLanding:
            landingMenuRender(&gameMenu->landingMenu, renderState, task);
            break;
        case GameMenuStateNewGame:
            newGameRender(&gameMenu->newGameMenu, renderState, task);
            break;
        case GameMenuStateLoadGame:
            loadGameRender(&gameMenu->loadGameMenu, renderState, task);
            break;
        case GameMenuStateSaveGame:
            saveGameRender(&gameMenu->saveGameMenu, renderState, task);
            break;
        case GameMenuStateOptions:
            optionsMenuRender(&gameMenu->optionsMenu, renderState, task);
            break;
        default:
            break;
    }
}