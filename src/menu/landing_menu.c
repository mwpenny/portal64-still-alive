#include "landing_menu.h"

#include "../font/font.h"
#include "../font/dejavusans.h"
#include "../controls/controller.h"

#include "../build/assets/materials/ui.h"

#define PORTAL_LOGO_X   30
#define PORTAL_LOGO_Y   74

#define PORTAL_LOGO_WIDTH  128
#define PORTAL_LOGO_P_WIDTH  15
#define PORTAL_LOGO_O_WIDTH  30
#define PORTAL_LOGO_HEIGHT 47

Gfx portal_logo_gfx[] = {
    gsSPTextureRectangle(
        PORTAL_LOGO_X << 2,
        PORTAL_LOGO_Y << 2,
        (PORTAL_LOGO_X + PORTAL_LOGO_P_WIDTH) << 2,
        (PORTAL_LOGO_Y + PORTAL_LOGO_HEIGHT) << 2,
        G_TX_RENDERTILE,
        0, 0,
        0x400, 0x400
    ),
    gsDPPipeSync(),
    gsDPSetEnvColor(60, 189, 236, 255),
    gsSPTextureRectangle(
        (PORTAL_LOGO_X + PORTAL_LOGO_P_WIDTH) << 2,
        PORTAL_LOGO_Y << 2,
        (PORTAL_LOGO_X + PORTAL_LOGO_P_WIDTH + PORTAL_LOGO_O_WIDTH) << 2,
        (PORTAL_LOGO_Y + PORTAL_LOGO_HEIGHT) << 2,
        G_TX_RENDERTILE,
        PORTAL_LOGO_P_WIDTH << 5, 0,
        0x400, 0x400
    ),
    gsDPPipeSync(),
    gsDPSetEnvColor(255, 255, 255, 255),
    gsSPTextureRectangle(
        (PORTAL_LOGO_X + PORTAL_LOGO_P_WIDTH + PORTAL_LOGO_O_WIDTH) << 2,
        PORTAL_LOGO_Y << 2,
        (PORTAL_LOGO_X + PORTAL_LOGO_WIDTH) << 2,
        (PORTAL_LOGO_Y + PORTAL_LOGO_HEIGHT) << 2,
        G_TX_RENDERTILE,
        (PORTAL_LOGO_P_WIDTH + PORTAL_LOGO_O_WIDTH) << 5, 0,
        0x400, 0x400
    ),
    gsSPEndDisplayList(),
};

void landingMenuInit(struct LandingMenu* landingMenu) {
    landingMenu->newGameText = menuBuildText(&gDejaVuSansFont, "NEW GAME", 30, 132);
    landingMenu->loadGameText = menuBuildText(&gDejaVuSansFont, "LOAD GAME", 30, 148);
    landingMenu->optionsText = menuBuildText(&gDejaVuSansFont, "OPTIONS", 30, 164);

    landingMenu->selectedItem = 0;
}

enum MainMenuState landingMenuUpdate(struct LandingMenu* landingMenu) {
    if ((controllerGetDirectionDown(0) & ControllerDirectionUp) != 0 && landingMenu->selectedItem > 0) {
        --landingMenu->selectedItem;
    }

    if ((controllerGetDirectionDown(0) & ControllerDirectionDown) != 0 && landingMenu->selectedItem < 2) {
        ++landingMenu->selectedItem;
    }

    if (controllerGetButtonDown(0, A_BUTTON)) {
        switch (landingMenu->selectedItem)
        {
            case 0:
                return MainMenuStateNewGame;
                break;
        }
    }

    return MainMenuStateLanding;
}

void landingMenuRender(struct LandingMenu* landingMenu, struct RenderState* renderState, struct GraphicsTask* task) {
    gSPDisplayList(renderState->dl++, ui_material_list[DEFAULT_UI_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[PORTAL_LOGO_INDEX]);
    gSPDisplayList(renderState->dl++, portal_logo_gfx);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[PORTAL_LOGO_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[DEJAVU_SANS_INDEX]);
    gDPPipeSync(renderState->dl++);
    menuSetRenderColor(renderState, landingMenu->selectedItem == 0, &gSelectionGray, &gColorWhite);
    gSPDisplayList(renderState->dl++, landingMenu->newGameText);
    gDPPipeSync(renderState->dl++);
    menuSetRenderColor(renderState, landingMenu->selectedItem == 1, &gSelectionGray, &gColorWhite);
    gSPDisplayList(renderState->dl++, landingMenu->loadGameText);
    gDPPipeSync(renderState->dl++);
    menuSetRenderColor(renderState, landingMenu->selectedItem == 2, &gSelectionGray, &gColorWhite);
    gSPDisplayList(renderState->dl++, landingMenu->optionsText);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_INDEX]);
}