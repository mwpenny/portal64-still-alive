#include "controls.h"

#include "../font/dejavusans.h"
#include "../font/font.h"

#include "../build/assets/materials/ui.h"

#define CONTROL_ROW_HEIGHT  14

#define CONTROLS_Y      54
#define CONTROLS_WIDTH  252
#define CONTROLS_HEIGHT 100

#define CONTROLS_X      ((SCREEN_WD - CONTROLS_WIDTH) / 2)

struct ControlActionDataRow {
    char* name;
    enum ControllerAction action;
    char* header;
};

struct ControlActionDataRow gControllerDataRows[] = {
    {"Move", ControllerActionMove, "Movement"},
    {"Look", ControllerActionRotate, NULL},
    {"Jump", ControllerActionJump, NULL},
    {"Duck", ControllerActionDuck, NULL},

    {"Fire blue portal", ControllerActionOpenPortal0, "Combat"},
    {"Fire red portal", ControllerActionOpenPortal1, NULL},
    {"Use item", ControllerActionUseItem, NULL},

    {"Pause", ControllerActionPause, "Misc"},

    {"Look forward", ControllerActionLookForward, "Misc Movement"},
    {"Look backward", ControllerActionLookForward, NULL},
};

void controlsRerenderRow(struct ControlsMenuRow* row, struct ControlActionDataRow* data, int x, int y) {
    fontRender(&gDejaVuSansFont, data->name, x, y, row->actionText);
}

void controlsInitRow(struct ControlsMenuRow* row, struct ControlActionDataRow* data) {
    row->actionText = menuBuildText(&gDejaVuSansFont, data->name, 0, 0);

    Gfx* dl = row->sourceIcons;
    gSPEndDisplayList(dl++);
}

void controlsRerender(struct ControlsMenu* controlsMenu) {
    int y = CONTROLS_Y;

    for (int i = 0; i < ControllerActionCount; ++i) {
        controlsRerenderRow(&controlsMenu->actionRows[i], &gControllerDataRows[i], CONTROLS_X, y);

        y += CONTROL_ROW_HEIGHT;
    }
}

void controlsMenuInit(struct ControlsMenu* controlsMenu) {
    for (int i = 0; i < ControllerActionCount; ++i) {
        controlsInitRow(&controlsMenu->actionRows[i], &gControllerDataRows[i]);
    }

    controlsRerender(controlsMenu);

    controlsMenu->scrollOutline = menuBuildOutline(CONTROLS_X, CONTROLS_Y, CONTROLS_WIDTH, CONTROLS_HEIGHT, 1);
}

void controlsMenuRender(struct ControlsMenu* controlsMenu, struct RenderState* renderState, struct GraphicsTask* task) {

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);
    gDPFillRectangle(renderState->dl++, CONTROLS_X, CONTROLS_Y, CONTROLS_X + CONTROLS_WIDTH, CONTROLS_Y + CONTROLS_HEIGHT);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_ENV_INDEX]);
    gSPDisplayList(renderState->dl++, controlsMenu->scrollOutline);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_ENV_INDEX]);

    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, CONTROLS_X, CONTROLS_Y, CONTROLS_X + CONTROLS_WIDTH, CONTROLS_Y + CONTROLS_HEIGHT);

    gSPDisplayList(renderState->dl++, ui_material_list[DEJAVU_SANS_INDEX]);
    for (int i = 0; i < ControllerActionCount; ++i) {
        gSPDisplayList(renderState->dl++, controlsMenu->actionRows[i].actionText);
    }
    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_INDEX]);

    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WD, SCREEN_HT);
}