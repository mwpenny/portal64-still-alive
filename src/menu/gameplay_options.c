#include "gameplay_options.h"

#include "../controls/controller.h"
#include "../font/dejavusans.h"
#include "../audio/soundplayer.h"
#include "../savefile/savefile.h"

#include "../build/src/audio/subtitles.h"

#include "../main.h"

#define MENU_Y      54
#define MENU_WIDTH  252
#define MENU_HEIGHT 124
#define MENU_X      ((SCREEN_WD - MENU_WIDTH) / 2)

struct MenuElementParams gGameplayMenuParams[] = {
    {
        .type = MenuElementTypeCheckbox,
        .x = MENU_X + 8, 
        .y = MENU_Y + 8,
        .params = {
            .checkbox = {
                .font = &gDejaVuSansFont,
                .messageId = GAMEUI_MOVINGPORTALS,
            },
        },
        .selectionIndex = GameplayOptionMovingPortals,
    },
    {
        .type = MenuElementTypeCheckbox,
        .x = MENU_X + 8, 
        .y = MENU_Y + 28,
        .params = {
            .checkbox = {
                .font = &gDejaVuSansFont,
                .messageId = GAMEUI_PORTALFUNNEL,
            },
        },
        .selectionIndex = GameplayOptionPortalFunneling,
    },
    {
        .type = MenuElementTypeText,
        .x = MENU_X + 8, 
        .y = MENU_Y + 48,
        .params = {
            .text = {
                .font = &gDejaVuSansFont,
                .messageId = GAMEUI_PORTALDEPTHLABEL,
            },
        },
        .selectionIndex = GameplayOptionPortalRenderDepth,
    },
    {
        .type = MenuElementTypeSlider,
        .x = MENU_X + 8, 
        .y = MENU_Y + 64,
        .params = {
            .slider = {
                .width = 232,
                .numberOfTicks = PORTAL_RENDER_DEPTH_MAX,
                .discrete = 1,
            },
        },
        .selectionIndex = GameplayOptionPortalRenderDepth,
    },
};

void gameplayOptionsAction(void* data, int selection, struct MenuAction* action) {
    switch (selection) {
        case GameplayOptionMovingPortals:
            if (action->state.checkbox.isChecked) {
                gSaveData.controls.flags |= ControlSaveMoveablePortals;
            } else {
                gSaveData.controls.flags &= ~ControlSaveMoveablePortals;
            }
            break;
        case GameplayOptionPortalFunneling:
            if (action->state.checkbox.isChecked) {
                gSaveData.controls.flags |= ControlSavePortalFunneling;
            } else {
                gSaveData.controls.flags &= ~ControlSavePortalFunneling;
            }
            break;
        case GameplayOptionPortalRenderDepth:
            gSaveData.controls.portalRenderDepth = action->state.iSlider.value;
            break;
    }
}

#define MOVING_PORTAL_INDEX 0
#define PORTAL_FUNNELING_INDEX 1
#define PORTAL_DEPTH    3

void gameplayOptionsInit(struct GameplayOptions* gameplayOptions) {
    menuBuilderInit(
        &gameplayOptions->menuBuilder,
        gGameplayMenuParams,
        sizeof(gGameplayMenuParams) / sizeof(*gGameplayMenuParams),
        GameplayOptionCount,
        gameplayOptionsAction,
        gameplayOptions
    );
 
    menuBuilderSetCheckbox(&gameplayOptions->menuBuilder.elements[MOVING_PORTAL_INDEX], (gSaveData.controls.flags & ControlSaveMoveablePortals) != 0);
    menuBuilderSetCheckbox(&gameplayOptions->menuBuilder.elements[PORTAL_FUNNELING_INDEX], (gSaveData.controls.flags & ControlSavePortalFunneling) != 0);

    menuBuilderSetISlider(&gameplayOptions->menuBuilder.elements[PORTAL_DEPTH], gSaveData.controls.portalRenderDepth);
}

void gameplayOptionsRebuildText(struct GameplayOptions* gameplayOptions) {
    menuBuilderRebuildText(&gameplayOptions->menuBuilder);
}

enum InputCapture gameplayOptionsUpdate(struct GameplayOptions* gameplayOptions) {
    return menuBuilderUpdate(&gameplayOptions->menuBuilder);
}

void gameplayOptionsRender(struct GameplayOptions* gameplayOptions, struct RenderState* renderState, struct GraphicsTask* task) {
    menuBuilderRender(&gameplayOptions->menuBuilder, renderState);
}
