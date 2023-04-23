#include "options_menu.h"

#include "../font/font.h"
#include "../font/dejavusans.h"

#include "../build/assets/materials/ui.h"

#include "../controls/controller.h"

#define OPTIONS_LEFT       40
#define OPTIONS_TOP        45

void optionsMenuInit(struct OptionsMenu* options) {
    options->menuOutline = menuBuildBorder(OPTIONS_LEFT, OPTIONS_TOP, SCREEN_WD - OPTIONS_LEFT * 2, SCREEN_HT - OPTIONS_TOP * 2);

    options->optionsText = menuBuildText(&gDejaVuSansFont, "OPTIONS", 48, 48);
}

enum MainMenuState optionsMenuUpdate(struct OptionsMenu* options) {
    if (controllerGetButtonDown(0, B_BUTTON)) {
        return MainMenuStateLanding;
    }

    return MainMenuStateOptions;
}

void optionsMenuRender(struct OptionsMenu* options, struct RenderState* renderState, struct GraphicsTask* task) {
    gSPDisplayList(renderState->dl++, ui_material_list[DEFAULT_UI_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);
    gDPFillRectangle(renderState->dl++, 0, 0, SCREEN_WD, SCREEN_HT);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[ROUNDED_CORNERS_INDEX]);
    gSPDisplayList(renderState->dl++, options->menuOutline);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[ROUNDED_CORNERS_INDEX]);
}