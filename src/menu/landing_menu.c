#include "landing_menu.h"

#include "../font/font.h"
#include "../font/dejavusans.h"
#include "../controls/controller.h"
#include "../util/memory.h"
#include "../audio/soundplayer.h"

#include "../build/assets/materials/ui.h"
#include "../build/src/audio/clips.h"

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

void landingMenuInit(struct LandingMenu* landingMenu, struct LandingMenuOption* options, int optionCount, int darkenBackground) {    
    landingMenu->optionText = malloc(sizeof(Gfx*) * optionCount);

    int y = 132;

    int stride = optionCount > 4 ? 12 : 16;

    for (int i = 0; i < optionCount; ++i) {
        landingMenu->optionText[i] = menuBuildText(&gDejaVuSansFont, options[i].message, 30, y);
        y += stride;
    }

    landingMenu->options = options;
    landingMenu->selectedItem = 0;
    landingMenu->optionCount = optionCount;
    landingMenu->darkenBackground = darkenBackground;
}

struct LandingMenuOption* landingMenuUpdate(struct LandingMenu* landingMenu) {
    if ((controllerGetDirectionDown(0) & ControllerDirectionUp) != 0 && landingMenu->selectedItem > 0) {
        --landingMenu->selectedItem;
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 0.5f, NULL, NULL);
    }

    if ((controllerGetDirectionDown(0) & ControllerDirectionDown) != 0 && landingMenu->selectedItem + 1 < landingMenu->optionCount) {
        ++landingMenu->selectedItem;
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 0.5f, NULL, NULL);
    }

    if (controllerGetButtonDown(0, A_BUTTON)) {
        soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL);
        return &landingMenu->options[landingMenu->selectedItem];
    }

    return NULL;
}

void landingMenuRender(struct LandingMenu* landingMenu, struct RenderState* renderState, struct GraphicsTask* task) {
    gSPDisplayList(renderState->dl++, ui_material_list[DEFAULT_UI_INDEX]);

    if (landingMenu->darkenBackground) {
        gSPDisplayList(renderState->dl++, ui_material_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);
        gDPFillRectangle(renderState->dl++, 0, 0, SCREEN_WD, SCREEN_HT);
        gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);
    }

    gSPDisplayList(renderState->dl++, ui_material_list[PORTAL_LOGO_INDEX]);
    gSPDisplayList(renderState->dl++, portal_logo_gfx);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[PORTAL_LOGO_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[DEJAVU_SANS_INDEX]);
    for (int i = 0; i < landingMenu->optionCount; ++i) {
        gDPPipeSync(renderState->dl++);
        menuSetRenderColor(renderState, landingMenu->selectedItem == i, &gSelectionGray, &gColorWhite);
        gSPDisplayList(renderState->dl++, landingMenu->optionText[i]);
    }
    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_INDEX]);
}