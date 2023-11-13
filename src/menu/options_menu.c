#include "options_menu.h"

#include "../font/font.h"
#include "../font/dejavusans.h"

#include "../savefile/savefile.h"

#include "../build/assets/materials/ui.h"

#include "../controls/controller.h"
#include "../build/src/audio/clips.h"
#include "../build/src/audio/subtitles.h"


struct Tab gOptionTabs[] = {
    {
        .messageId = OPTIONS_CONTROLS,
    },
    {
        .messageId = GAMEUI_JOYSTICK_N64,
    },
    {
        .messageId = GAMEUI_AUDIO,
    },
    {
        .messageId = GAMEUI_VIDEO,
    },
    {
        .messageId = GAMEUI_PORTAL,
    },
};

#define MENU_WIDTH  280
#define MENU_HEIGHT 200

#define MENU_LEFT   ((SCREEN_WD - MENU_WIDTH) / 2)
#define MENU_TOP    ((SCREEN_HT - MENU_HEIGHT) / 2)

#define OPTIONS_PADDING 8

void optionsMenuInit(struct OptionsMenu* options) {
    options->menuOutline = menuBuildBorder(MENU_LEFT, MENU_TOP, MENU_WIDTH, MENU_HEIGHT);

    options->optionsText = menuBuildText(&gDejaVuSansFont, "OPTIONS", 48, 48);

    tabsInit(
        &options->tabs, 
        gOptionTabs, 
        sizeof(gOptionTabs) / sizeof(*gOptionTabs), 
        &gDejaVuSansFont,
        MENU_LEFT + OPTIONS_PADDING, MENU_TOP + OPTIONS_PADDING,
        MENU_WIDTH - OPTIONS_PADDING * 2, MENU_HEIGHT - OPTIONS_PADDING * 2
    );

    controlsMenuInit(&options->controlsMenu);
    joystickOptionsInit(&options->joystickOptions);
    audioOptionsInit(&options->audioOptions);
    videoOptionsInit(&options->videoOptions);
    gameplayOptionsInit(&options->gameplayOptions);
}

void optionsMenuRebuildText(struct OptionsMenu* options) {
    controlsRebuildtext(&options->controlsMenu);
    joystickOptionsRebuildText(&options->joystickOptions);
    audioOptionsRebuildtext(&options->audioOptions);
    videoOptionsRebuildtext(&options->videoOptions);
    gameplayOptionsRebuildText(&options->gameplayOptions);
    tabsRebuildText(&options->tabs);
}

enum InputCapture optionsMenuUpdate(struct OptionsMenu* options) {
    enum InputCapture result = InputCapturePass;

    switch (options->tabs.selectedTab) {
        case OptionsMenuTabsControlMapping:
            result = controlsMenuUpdate(&options->controlsMenu);
            break;
        case OptionsMenuTabsControlJoystick:
            result = joystickOptionsUpdate(&options->joystickOptions);
            break;
        case OptionsMenuTabsAudio:
            result = audioOptionsUpdate(&options->audioOptions);
            break;
        case OptionsMenuTabsVideo:
            result = videoOptionsUpdate(&options->videoOptions);
            break;
        case OptionsMenuTabsGameplay:
            result = gameplayOptionsUpdate(&options->gameplayOptions);
            break;
    }

    if (result == InputCaptureExit || controllerGetButtonDown(0, B_BUTTON)) {
        savefileSave();
        return InputCaptureExit;
    } else if (result != InputCapturePass) {
        return result;
    }

    if (controllerGetButtonDown(0, Z_TRIG | L_TRIG)) {
        if (options->tabs.selectedTab == 0) {
            tabsSetSelectedTab(&options->tabs, OptionsMenuTabsCount - 1);
        } else {
            tabsSetSelectedTab(&options->tabs, options->tabs.selectedTab - 1);
        }

        tabsSetSelectedTab(&options->tabs, options->tabs.selectedTab);
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
    }

    if (controllerGetButtonDown(0, R_TRIG)) {
        if (options->tabs.selectedTab == OptionsMenuTabsCount - 1) {
            tabsSetSelectedTab(&options->tabs, 0);
        } else {
            tabsSetSelectedTab(&options->tabs, options->tabs.selectedTab + 1);
        }
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);

    }

    return InputCapturePass;
}

void optionsMenuRender(struct OptionsMenu* options, struct RenderState* renderState, struct GraphicsTask* task) {
    gSPDisplayList(renderState->dl++, ui_material_list[DEFAULT_UI_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);
    gDPFillRectangle(renderState->dl++, 0, 0, SCREEN_WD, SCREEN_HT);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[ROUNDED_CORNERS_INDEX]);
    gSPDisplayList(renderState->dl++, options->menuOutline);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[ROUNDED_CORNERS_INDEX]);

    gDPSetScissor(renderState->dl++, 
        G_SC_NON_INTERLACE, 
        MENU_LEFT + OPTIONS_PADDING, 
        0,
        MENU_LEFT + MENU_WIDTH - OPTIONS_PADDING, 
        SCREEN_HT
    );

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_ENV_INDEX]);
    gSPDisplayList(renderState->dl++, options->tabs.tabOutline);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_ENV_INDEX]);

    struct PrerenderedTextBatch* batch = prerenderedBatchStart();
    tabsRenderText(&options->tabs, batch);
    renderState->dl = prerenderedBatchFinish(batch, gDejaVuSansImages, renderState->dl);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_0_INDEX]);

    gDPSetScissor(renderState->dl++, 
        G_SC_NON_INTERLACE, 
        0, 0,
        SCREEN_WD, SCREEN_HT
    );

    switch (options->tabs.selectedTab) {
        case OptionsMenuTabsControlMapping:
            controlsMenuRender(&options->controlsMenu, renderState, task);
            break;
        case OptionsMenuTabsControlJoystick:
            joystickOptionsRender(&options->joystickOptions, renderState, task);
            break;
        case OptionsMenuTabsAudio:
            audioOptionsRender(&options->audioOptions, renderState, task);
            break;
        case OptionsMenuTabsVideo:
            videoOptionsRender(&options->videoOptions, renderState, task);
            break;
        case OptionsMenuTabsGameplay:
            gameplayOptionsRender(&options->gameplayOptions, renderState, task);
            break;
    }
}
