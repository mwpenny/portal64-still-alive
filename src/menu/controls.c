#include "controls.h"

#include "../font/dejavusans.h"
#include "../font/font.h"
#include "../controls/controller.h"

#include "../build/assets/materials/ui.h"

#define CONTROL_ROW_HEIGHT  14

#define CONTROLS_Y      54
#define CONTROLS_WIDTH  252
#define CONTROLS_HEIGHT 124

#define CONTROLS_X      ((SCREEN_WD - CONTROLS_WIDTH) / 2)

#define ROW_PADDING         8
#define SEPARATOR_PADDING   8
#define HEADER_PADDING      4
#define TOP_PADDING         4

#define SEPARATOR_SPACE     3


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
    fontRender(&gDejaVuSansFont, data->name, x + ROW_PADDING, y, row->actionText);
    row->y = y;
}

void controlsInitRow(struct ControlsMenuRow* row, struct ControlActionDataRow* data) {
    row->actionText = menuBuildText(&gDejaVuSansFont, data->name, 0, 0);

    Gfx* dl = row->sourceIcons;
    gSPEndDisplayList(dl++);
}

void controlsRerenderHeader(struct ControlsMenuHeader* header, char* message, int x, int y) {
    header->headerText = menuBuildText(&gDejaVuSansFont, message, x + HEADER_PADDING, y);
}

void controlsInitHeader(struct ControlsMenuHeader* header, char* message) {
    header->headerText = menuBuildText(&gDejaVuSansFont, message, 0, 0);
}

void controlsRerender(struct ControlsMenu* controlsMenu) {
    int y = CONTROLS_Y + controlsMenu->scrollOffset;
    int currentHeader = 0;

    Gfx* headerSeparators = controlsMenu->headerSeparators;

    for (int i = 0; i < ControllerActionCount; ++i) {
        if (gControllerDataRows[i].header && currentHeader < MAX_CONTROLS_SECTIONS) {
            y += TOP_PADDING;
            controlsRerenderHeader(&controlsMenu->headers[currentHeader], gControllerDataRows[i].header, CONTROLS_X, y);
            y += CONTROL_ROW_HEIGHT;

            if (y > CONTROLS_Y + 1 && y < CONTROLS_Y + CONTROLS_HEIGHT - 1) {
                gDPFillRectangle(headerSeparators++, CONTROLS_X + SEPARATOR_PADDING, y, CONTROLS_X + CONTROLS_WIDTH - SEPARATOR_PADDING * 2, y + 1);
            }
            y += SEPARATOR_SPACE;
            ++currentHeader;
        }

        controlsRerenderRow(&controlsMenu->actionRows[i], &gControllerDataRows[i], CONTROLS_X, y);

        y += CONTROL_ROW_HEIGHT;
    }

    gSPEndDisplayList(headerSeparators++);
}

void controlsMenuInit(struct ControlsMenu* controlsMenu) {
    int currentHeader = 0;

    for (int i = 0; i < ControllerActionCount; ++i) {
        controlsInitRow(&controlsMenu->actionRows[i], &gControllerDataRows[i]);

        if (gControllerDataRows[i].header && currentHeader < MAX_CONTROLS_SECTIONS) {
            controlsInitHeader(&controlsMenu->headers[currentHeader], gControllerDataRows[i].header);
            ++currentHeader;
        }
    }

    for (; currentHeader < MAX_CONTROLS_SECTIONS; ++currentHeader) {
        controlsMenu->headers[currentHeader].headerText = NULL;
    }

    controlsMenu->selectedRow = 0;
    controlsMenu->scrollOffset = 0;

    controlsRerender(controlsMenu);

    controlsMenu->scrollOutline = menuBuildOutline(CONTROLS_X, CONTROLS_Y, CONTROLS_WIDTH, CONTROLS_HEIGHT, 1);
}

void controlsMenuUpdate(struct ControlsMenu* controlsMenu) {
    int controllerDir = controllerGetDirectionDown(0);
    if (controllerDir & ControllerDirectionDown) {
        controlsMenu->selectedRow = controlsMenu->selectedRow + 1;

        if (controlsMenu->selectedRow == ControllerActionCount) {
            controlsMenu->selectedRow = 0;
        }
    }

    if (controllerDir & ControllerDirectionUp) {
        controlsMenu->selectedRow = controlsMenu->selectedRow - 1;

        if (controlsMenu->selectedRow < 0) {
            controlsMenu->selectedRow = ControllerActionCount - 1;
        }
    }

    if (controlsMenu->selectedRow >= 0 && controlsMenu->selectedRow < ControllerActionCount) {
        struct ControlsMenuRow* selectedAction = &controlsMenu->actionRows[controlsMenu->selectedRow];
        int newScroll = controlsMenu->scrollOffset;
        int topY = selectedAction->y;
        int bottomY = topY + CONTROL_ROW_HEIGHT + TOP_PADDING;

        if (gControllerDataRows[controlsMenu->selectedRow].header) {
            topY -= CONTROL_ROW_HEIGHT + TOP_PADDING + SEPARATOR_SPACE;
        }

        if (topY < CONTROLS_Y) {
            newScroll = CONTROLS_Y - (topY - controlsMenu->scrollOffset);
        }

        if (bottomY > CONTROLS_Y + CONTROLS_HEIGHT) {
            newScroll = (CONTROLS_Y + CONTROLS_HEIGHT) - (bottomY - controlsMenu->scrollOffset);
        }

        if (newScroll != controlsMenu->scrollOffset) {
            controlsMenu->scrollOffset = newScroll;
            controlsRerender(controlsMenu);
        }
    }
}

void controlsMenuRender(struct ControlsMenu* controlsMenu, struct RenderState* renderState, struct GraphicsTask* task) {

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);
    gDPFillRectangle(renderState->dl++, CONTROLS_X, CONTROLS_Y, CONTROLS_X + CONTROLS_WIDTH, CONTROLS_Y + CONTROLS_HEIGHT);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_ENV_INDEX]);
    gSPDisplayList(renderState->dl++, controlsMenu->scrollOutline);
    gDPSetEnvColor(renderState->dl++, 0, 0, 0, 255);
    gSPDisplayList(renderState->dl++, controlsMenu->headerSeparators);

    if (controlsMenu->selectedRow >= 0 && controlsMenu->selectedRow < ControllerActionCount) {
        struct ControlsMenuRow* selectedAction = &controlsMenu->actionRows[controlsMenu->selectedRow];

        gDPPipeSync(renderState->dl++);
        gDPSetEnvColor(renderState->dl++, gSelectionOrange.r, gSelectionOrange.g, gSelectionOrange.b, gSelectionOrange.a);
        gDPFillRectangle(
            renderState->dl++, 
            CONTROLS_X + ROW_PADDING, 
            selectedAction->y, 
            CONTROLS_X + CONTROLS_WIDTH - ROW_PADDING * 2, 
            selectedAction->y + CONTROL_ROW_HEIGHT - 2
        );
    }

    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_ENV_INDEX]);

    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, CONTROLS_X, CONTROLS_Y, CONTROLS_X + CONTROLS_WIDTH, CONTROLS_Y + CONTROLS_HEIGHT);

    gSPDisplayList(renderState->dl++, ui_material_list[DEJAVU_SANS_INDEX]);
    for (int i = 0; i < ControllerActionCount; ++i) {
        if (controlsMenu->selectedRow == i) {
            gDPPipeSync(renderState->dl++);
            gDPSetEnvColor(renderState->dl++, 0, 0, 0, 255);
        }

        gSPDisplayList(renderState->dl++, controlsMenu->actionRows[i].actionText);

        if (controlsMenu->selectedRow == i) {
            gDPPipeSync(renderState->dl++);
            gDPSetEnvColor(renderState->dl++, 255, 255, 255, 255);
        }
    }
    for (int i = 0; i < MAX_CONTROLS_SECTIONS; ++i) {
        if (!controlsMenu->headers[i].headerText) {
            break;
        }
        gSPDisplayList(renderState->dl++, controlsMenu->headers[i].headerText);
    }
    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_INDEX]);

    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WD, SCREEN_HT);
}