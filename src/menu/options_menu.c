#include "options_menu.h"

#include "../font/font.h"
#include "../font/dejavusans.h"

#include "../savefile/savefile.h"

#include "../build/assets/materials/ui.h"

#include "../controls/controller.h"


struct Tab gOptionTabs[] = {
    {
        .message = "Controls",
    },
    {
        .message = "Joystick",
    },
    {
        .message = "Audio",
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
}

enum MenuDirection optionsMenuUpdate(struct OptionsMenu* options) {
    enum MenuDirection menuDirection = MenuDirectionStay;

    switch (options->tabs.selectedTab) {
        case OptionsMenuTabsControlMapping:
            menuDirection = controlsMenuUpdate(&options->controlsMenu);
            break;
        case OptionsMenuTabsControlJoystick:
            menuDirection = joystickOptionsUpdate(&options->joystickOptions);
            break;
        case OptionsMenuTabsAudio:
            menuDirection = audioOptionsUpdate(&options->audioOptions);
            break;
    }

    if (menuDirection == MenuDirectionUp) {
        savefileSave();
        return MenuDirectionUp;
    }

    if (menuDirection == MenuDirectionLeft) {
        if (options->tabs.selectedTab == 0) {
            tabsSetSelectedTab(&options->tabs, OptionsMenuTabsCount - 1);
        } else {
            tabsSetSelectedTab(&options->tabs, options->tabs.selectedTab - 1);
        }

        tabsSetSelectedTab(&options->tabs, options->tabs.selectedTab);
    }

    if (menuDirection == MenuDirectionRight) {
        if (options->tabs.selectedTab == OptionsMenuTabsCount - 1) {
            tabsSetSelectedTab(&options->tabs, 0);
        } else {
            tabsSetSelectedTab(&options->tabs, options->tabs.selectedTab + 1);
        }

    }

    return MenuDirectionStay;
}

void optionsMenuRender(struct OptionsMenu* options, struct RenderState* renderState, struct GraphicsTask* task) {
    gSPDisplayList(renderState->dl++, ui_material_list[DEFAULT_UI_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);
    gDPFillRectangle(renderState->dl++, 0, 0, SCREEN_WD, SCREEN_HT);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[ROUNDED_CORNERS_INDEX]);
    gSPDisplayList(renderState->dl++, options->menuOutline);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[ROUNDED_CORNERS_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_ENV_INDEX]);
    gSPDisplayList(renderState->dl++, options->tabs.tabOutline);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_ENV_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[DEJAVU_SANS_INDEX]);
    renderState->dl = tabsRenderText(&options->tabs, renderState->dl);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_INDEX]);

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
    }
}