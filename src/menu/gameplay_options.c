#include "gameplay_options.h"

#include "../controls/controller.h"
#include "../font/dejavusans.h"
#include "../audio/soundplayer.h"
#include "../savefile/savefile.h"

#include "../build/assets/materials/ui.h"
#include "../build/src/audio/clips.h"

#define GAMEPLAY_Y      54
#define GAMEPLAY_WIDTH  252
#define GAMEPLAY_HEIGHT 124

#define SCROLL_TICKS        9
#define SCROLL_INTERVALS    (SCROLL_TICKS - 1)

#define GAMEPLAY_X      ((SCREEN_WD - GAMEPLAY_WIDTH) / 2)

void gameplayOptionsInit(struct GameplayOptions* gameplayOptions) {
    gameplayOptions->selectedItem = GameplayOptionMovingPortals;

    gameplayOptions->movingPortals = menuBuildCheckbox(&gDejaVuSansFont, "Movable Portals", GAMEPLAY_X + 8, GAMEPLAY_Y + 8);
    gameplayOptions->wideScreen = menuBuildCheckbox(&gDejaVuSansFont, "Wide Screen", GAMEPLAY_X + 8, GAMEPLAY_Y + 28);

    gameplayOptions->movingPortals.checked = (gSaveData.controls.flags & ControlSaveFlagsInvert) != 0;
    gameplayOptions->wideScreen.checked = (gSaveData.controls.flags & ControlSaveWideScreen) != 0;
}

enum MenuDirection gameplayOptionsUpdate(struct GameplayOptions* gameplayOptions) {
    int controllerDir = controllerGetDirectionDown(0);

    if (controllerGetButtonDown(0, B_BUTTON)) {
        return MenuDirectionUp;
    }

    if (controllerDir & ControllerDirectionDown) {
        ++gameplayOptions->selectedItem;

        if (gameplayOptions->selectedItem == GameplayOptionCount) {
            gameplayOptions->selectedItem = 0;
        }
    }

    if (controllerDir & ControllerDirectionUp) {
        if (gameplayOptions->selectedItem == 0) {
            gameplayOptions->selectedItem = GameplayOptionCount - 1;
        } else {
            --gameplayOptions->selectedItem;
        }
    }

    switch (gameplayOptions->selectedItem) {
        case GameplayOptionMovingPortals:
            if (controllerGetButtonDown(0, A_BUTTON)) {
                gameplayOptions->movingPortals.checked = !gameplayOptions->movingPortals.checked;
                soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL);

                if (gameplayOptions->movingPortals.checked) {
                    gSaveData.controls.flags |= ControlSaveMoveablePortals;
                } else {
                    gSaveData.controls.flags &= ~ControlSaveMoveablePortals;
                }
            }

            break;
        case GameplayOptionWideScreen:
            if (controllerGetButtonDown(0, A_BUTTON)) {
                gameplayOptions->wideScreen.checked = !gameplayOptions->wideScreen.checked;
                soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL);

                if (gameplayOptions->wideScreen.checked) {
                    gSaveData.controls.flags |= ControlSaveWideScreen;
                } else {
                    gSaveData.controls.flags &= ~ControlSaveWideScreen;
                }
            }
        break;
    }

    if (controllerDir & ControllerDirectionLeft) {
        return MenuDirectionLeft;
    }

    if (controllerDir & ControllerDirectionRight) {
        return MenuDirectionRight;
    }

    return MenuDirectionStay;
}

void gameplayOptionsRender(struct GameplayOptions* gameplayOptions, struct RenderState* renderState, struct GraphicsTask* task) {
    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_ENV_INDEX]);
    
    gSPDisplayList(renderState->dl++, gameplayOptions->movingPortals.outline);
    renderState->dl = menuCheckboxRender(&gameplayOptions->movingPortals, renderState->dl);
    
    gSPDisplayList(renderState->dl++, gameplayOptions->wideScreen.outline);
    renderState->dl = menuCheckboxRender(&gameplayOptions->wideScreen, renderState->dl);

    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_ENV_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[DEJAVU_SANS_INDEX]);

    gDPPipeSync(renderState->dl++);
    menuSetRenderColor(renderState, gameplayOptions->selectedItem == GameplayOptionMovingPortals, &gSelectionGray, &gColorWhite);
    gSPDisplayList(renderState->dl++, gameplayOptions->movingPortals.text);

    menuSetRenderColor(renderState, gameplayOptions->selectedItem == GameplayOptionWideScreen, &gSelectionGray, &gColorWhite);
    gSPDisplayList(renderState->dl++, gameplayOptions->wideScreen.text);

    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_INDEX]);
}