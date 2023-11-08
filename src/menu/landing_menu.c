#include "landing_menu.h"

#include "../font/font.h"
#include "../font/dejavusans.h"
#include "../controls/controller.h"
#include "../util/memory.h"
#include "../audio/soundplayer.h"

#include "../build/assets/materials/ui.h"
#include "../build/src/audio/clips.h"
#include "cheat_codes.h"
#include "./translations.h"

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

void landingMenuInitText(struct LandingMenu* landingMenu) {
    int y = 132;

    int stride = landingMenu->optionCount > 4 ? 12 : 16;

    for (int i = 0; i < landingMenu->optionCount; ++i) {
        landingMenu->optionText[i] = menuBuildPrerenderedText(&gDejaVuSansFont, translationsGet(landingMenu->options[i].messageId), 30, y, SCREEN_WD);
        y += stride;
    }
}

void landingMenuInit(struct LandingMenu* landingMenu, struct LandingMenuOption* options, int optionCount, int darkenBackground) {    
    landingMenu->optionText = malloc(sizeof(struct PrerenderedText*) * optionCount);
    landingMenu->options = options;
    landingMenu->selectedItem = 0;
    landingMenu->optionCount = optionCount;
    landingMenu->darkenBackground = darkenBackground;
    landingMenuInitText(landingMenu);
}

void landingMenuRebuildText(struct LandingMenu* landingMenu) {
    for (int i = 0; i < landingMenu->optionCount; ++i) {
        prerenderedTextFree(landingMenu->optionText[i]);
    }
    landingMenuInitText(landingMenu);
}

struct LandingMenuOption* landingMenuUpdate(struct LandingMenu* landingMenu) {
    enum ControllerDirection dir = controllerGetDirectionDown(0);

    if (dir & ControllerDirectionUp) {
        if (landingMenu->selectedItem > 0) {
            --landingMenu->selectedItem;
        } else { 
            landingMenu->selectedItem = landingMenu->optionCount - 1;
        }
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);

        cheatCodeEnterDirection(CheatCodeDirUp);
    }

    if (dir & ControllerDirectionDown) {
        if (landingMenu->selectedItem + 1 < landingMenu->optionCount) {
            ++landingMenu->selectedItem;
        } else {
            landingMenu->selectedItem = 0;
        }
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);

        cheatCodeEnterDirection(CheatCodeDirDown);
    }

    if (dir & ControllerDirectionLeft) {
        cheatCodeEnterDirection(CheatCodeDirLeft);
    }

    if (dir & ControllerDirectionRight) {
        cheatCodeEnterDirection(CheatCodeDirRight);
    }

    if (controllerGetButtonDown(0, A_BUTTON)) {
        soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
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

    struct PrerenderedTextBatch* batch = prerenderedBatchStart();
    for (int i = 0; i < landingMenu->optionCount; ++i) {
        prerenderedBatchAdd(batch, landingMenu->optionText[i], landingMenu->selectedItem == i ? &gSelectionGray: &gColorWhite);
    }
    renderState->dl = prerenderedBatchFinish(batch, gDejaVuSansImages, renderState->dl);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_0_INDEX]);

}
