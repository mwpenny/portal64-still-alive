#include "controls.h"

#include "../font/dejavusans.h"
#include "../font/font.h"
#include "../controls/controller.h"
#include "../audio/soundplayer.h"

#include "../build/assets/materials/ui.h"

#include "../build/src/audio/clips.h"

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

#define USE_DEFAULTS_X      190
#define USE_DEFAULTS_Y      186
#define USE_DEFAULTS_WIDTH  96
#define USE_DEFAULTS_HEIGHT 16

struct ControllerIcon {
    char x, y;
    char w, h;
};

enum ControllerButtonIcons {
    ControllerButtonIconsA,
    ControllerButtonIconsB,
    ControllerButtonIconsS,
    ControllerButtonIconsCU,
    ControllerButtonIconsCR,
    ControllerButtonIconsCD,
    ControllerButtonIconsCL,
    ControllerButtonIconsDU,
    ControllerButtonIconsDR,
    ControllerButtonIconsDD,
    ControllerButtonIconsDL,
    ControllerButtonIconsZ,
    ControllerButtonIconsR,
    ControllerButtonIconsL,
};

struct ControllerIcon gControllerButtonIcons[] = {
    [ControllerButtonIconsA] = {0, 0, 12, 12},
    [ControllerButtonIconsB] = {12, 0, 12, 12},
    [ControllerButtonIconsS] = {24, 0, 12, 12},

    [ControllerButtonIconsCU] = {0, 24, 12, 12},
    [ControllerButtonIconsCR] = {12, 24, 12, 12},
    [ControllerButtonIconsCD] = {24, 24, 12, 12},
    [ControllerButtonIconsCL] = {36, 24, 12, 12},

    [ControllerButtonIconsDU] = {0, 36, 12, 12},
    [ControllerButtonIconsDR] = {12, 36, 12, 12},
    [ControllerButtonIconsDD] = {24, 36, 12, 12},
    [ControllerButtonIconsDL] = {36, 36, 12, 12},

    [ControllerButtonIconsZ] = {0, 48, 12, 12},
    [ControllerButtonIconsR] = {12, 48, 12, 12},
    [ControllerButtonIconsL] = {24, 48, 12, 12},
};

char gControllerActionToButtonIcon[] = {
    [ControllerActionSourceAButton] = ControllerButtonIconsA,
    [ControllerActionSourceBButton] = ControllerButtonIconsB,
    [ControllerActionSourceStartButton] = ControllerButtonIconsS,

    [ControllerActionSourceCUpButton] = ControllerButtonIconsCU,
    [ControllerActionSourceCRightButton] = ControllerButtonIconsCR,
    [ControllerActionSourceCDownButton] = ControllerButtonIconsCD,
    [ControllerActionSourceCLeftButton] = ControllerButtonIconsCL,

    [ControllerActionSourceDUpButton] = ControllerButtonIconsDU,
    [ControllerActionSourceDRightButton] = ControllerButtonIconsDR,
    [ControllerActionSourceDDownButton] = ControllerButtonIconsDD,
    [ControllerActionSourceDLeftButton] = ControllerButtonIconsDL,

    [ControllerActionSourceZTrig] = ControllerButtonIconsZ,
    [ControllerActionSourceRTrig] = ControllerButtonIconsR,
    [ControllerActionSourceLTrig] = ControllerButtonIconsL,
};

enum ControllerDirectionIcons {
    ControllerDirectionIconsC,
    ControllerDirectionIconsD,
    ControllerDirectionIconsJ,
};

struct ControllerIcon gControllerDirectionIcons[] = {
    [ControllerDirectionIconsC] = {0, 12, 14, 12},
    [ControllerDirectionIconsD] = {14, 12, 12, 12},
    [ControllerDirectionIconsJ] = {26, 12, 15, 12},
};

char gControllerActionToDirectionIcon[] = {
    [ControllerActionSourceCUpButton] = ControllerDirectionIconsC,
    [ControllerActionSourceDUpButton] = ControllerDirectionIconsD,
    [ControllerActionSourceJoystick] = ControllerDirectionIconsJ,
};

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

void controlsRenderIcons(Gfx* dl, enum ControllerAction action, int y) {
    struct ControllerSourceWithController sources[MAX_SOURCES_PER_ACTION];

    int sourceCount = controllerSourcesForAction(action, sources, MAX_SOURCES_PER_ACTION);

    char* iconMapping;
    struct ControllerIcon* icons;

    int x = CONTROLS_X + CONTROLS_WIDTH - ROW_PADDING * 2;
    
    if (IS_DIRECTION_ACTION(action)) {
        iconMapping = gControllerActionToDirectionIcon;
        icons = gControllerDirectionIcons;
    } else {
        iconMapping = gControllerActionToButtonIcon;
        icons = gControllerButtonIcons;
    }

    for (int i = 0; i < sourceCount; ++i) {
        struct ControllerIcon icon = icons[(int)iconMapping[(int)sources[i].button]];

        x -= icon.w;
        gSPTextureRectangle(
            dl++, 
            x << 2, y << 2, 
            (x + icon.w) << 2, (y + icon.h) << 2, 
            G_TX_RENDERTILE, 
            icon.x << 5, icon.y << 5, 
            0x400, 0x400
        );
    }

    gSPEndDisplayList(dl++);
}

void controlsLayoutRow(struct ControlsMenuRow* row, struct ControlActionDataRow* data, int x, int y) {
    fontRender(&gDejaVuSansFont, data->name, x + ROW_PADDING, y, row->actionText);
    controlsRenderIcons(row->sourceIcons, data->action, y);
    row->y = y;
}

void controlsInitRow(struct ControlsMenuRow* row, struct ControlActionDataRow* data) {
    row->actionText = menuBuildText(&gDejaVuSansFont, data->name, 0, 0);

    Gfx* dl = row->sourceIcons;
    for (int i = 0; i < SOURCE_ICON_COUNT; ++i) {
        gSPEndDisplayList(dl++);
    }
}

void controlsLayoutHeader(struct ControlsMenuHeader* header, char* message, int x, int y) {
    header->headerText = menuBuildText(&gDejaVuSansFont, message, x + HEADER_PADDING, y);
}

void controlsInitHeader(struct ControlsMenuHeader* header, char* message) {
    header->headerText = menuBuildText(&gDejaVuSansFont, message, 0, 0);
}

void controlsLayout(struct ControlsMenu* controlsMenu) {
    int y = CONTROLS_Y + controlsMenu->scrollOffset;
    int currentHeader = 0;

    Gfx* headerSeparators = controlsMenu->headerSeparators;

    for (int i = 0; i < ControllerActionCount; ++i) {
        if (gControllerDataRows[i].header && currentHeader < MAX_CONTROLS_SECTIONS) {
            y += TOP_PADDING;
            controlsLayoutHeader(&controlsMenu->headers[currentHeader], gControllerDataRows[i].header, CONTROLS_X, y);
            y += CONTROL_ROW_HEIGHT;

            if (y > CONTROLS_Y + 1 && y < CONTROLS_Y + CONTROLS_HEIGHT - 1) {
                gDPFillRectangle(headerSeparators++, CONTROLS_X + SEPARATOR_PADDING, y, CONTROLS_X + CONTROLS_WIDTH - SEPARATOR_PADDING * 2, y + 1);
            }
            y += SEPARATOR_SPACE;
            ++currentHeader;
        }

        controlsLayoutRow(&controlsMenu->actionRows[i], &gControllerDataRows[i], CONTROLS_X, y);

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
    controlsMenu->waitingForAction = ControllerActionNone;

    controlsLayout(controlsMenu);

    controlsMenu->useDefaults = menuBuildButton(
        &gDejaVuSansFont, 
        "Use Defaults", 
        USE_DEFAULTS_X, USE_DEFAULTS_Y,
        USE_DEFAULTS_WIDTH, USE_DEFAULTS_HEIGHT
    );

    controlsMenu->scrollOutline = menuBuildOutline(CONTROLS_X, CONTROLS_Y, CONTROLS_WIDTH, CONTROLS_HEIGHT, 1);
}

enum MenuDirection controlsMenuUpdate(struct ControlsMenu* controlsMenu) {
    if (controlsMenu->waitingForAction != ControllerActionNone) {
        struct ControllerSourceWithController source = controllerReadAnySource();

        if (IS_VALID_SOURCE(source.button)) {
            controllerSetSource(controlsMenu->waitingForAction, source.button, source.controller);
            controlsMenu->waitingForAction = ControllerActionNone;

            controlsLayout(controlsMenu);

            soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL);
        }

        return MenuDirectionStay;
    }

    int controllerDir = controllerGetDirectionDown(0);
    if (controllerDir & ControllerDirectionDown) {
        controlsMenu->selectedRow = controlsMenu->selectedRow + 1;

        if (controlsMenu->selectedRow == ControllerActionCount + 1) {
            controlsMenu->selectedRow = 0;
        }
    }

    if (controllerDir & ControllerDirectionUp) {
        controlsMenu->selectedRow = controlsMenu->selectedRow - 1;

        if (controlsMenu->selectedRow < 0) {
            controlsMenu->selectedRow = ControllerActionCount;
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
            controlsLayout(controlsMenu);
        }
    }

    if (controllerGetButtonDown(0, A_BUTTON)) {
        if (controlsMenu->selectedRow >= 0 && controlsMenu->selectedRow < ControllerActionCount) {
            controlsMenu->waitingForAction = gControllerDataRows[controlsMenu->selectedRow].action;
        } else if (controlsMenu->selectedRow == ControllerActionCount) {
            controllerSetDefaultSource();
            controlsLayout(controlsMenu);
        }

        soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL);
    }

    if (controllerGetButtonDown(0, B_BUTTON)) {
        return MenuDirectionUp;
    }

    if (controllerDir & ControllerDirectionLeft) {
        return MenuDirectionLeft;
    }

    if (controllerDir & ControllerDirectionRight) {
        return MenuDirectionRight;
    }

    return MenuDirectionStay;
}

void controlsMenuRender(struct ControlsMenu* controlsMenu, struct RenderState* renderState, struct GraphicsTask* task) {
    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);
    gDPFillRectangle(renderState->dl++, CONTROLS_X, CONTROLS_Y, CONTROLS_X + CONTROLS_WIDTH, CONTROLS_Y + CONTROLS_HEIGHT);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_ENV_INDEX]);
    gSPDisplayList(renderState->dl++, controlsMenu->scrollOutline);
    gDPPipeSync(renderState->dl++);
    gDPSetEnvColor(renderState->dl++, 0, 0, 0, 255);
    renderStateInlineBranch(renderState, controlsMenu->headerSeparators);


    if (controlsMenu->selectedRow >= 0 && controlsMenu->selectedRow < ControllerActionCount) {
        struct ControlsMenuRow* selectedAction = &controlsMenu->actionRows[controlsMenu->selectedRow];

        gDPPipeSync(renderState->dl++);
        if (controlsMenu->waitingForAction != ControllerActionNone) {
            gDPSetEnvColor(renderState->dl++, gSelectionGray.r, gSelectionGray.g, gSelectionGray.b, gSelectionGray.a);    
        } else {
            gDPSetEnvColor(renderState->dl++, gSelectionOrange.r, gSelectionOrange.g, gSelectionOrange.b, gSelectionOrange.a);
        }
        gDPFillRectangle(
            renderState->dl++, 
            CONTROLS_X + ROW_PADDING, 
            selectedAction->y, 
            CONTROLS_X + CONTROLS_WIDTH - ROW_PADDING * 2, 
            selectedAction->y + CONTROL_ROW_HEIGHT - 2
        );
    }

    if (controlsMenu->selectedRow == ControllerActionCount) {
        gDPPipeSync(renderState->dl++);
        gDPSetEnvColor(renderState->dl++, gSelectionOrange.r, gSelectionOrange.g, gSelectionOrange.b, gSelectionOrange.a);
        gDPFillRectangle(
            renderState->dl++, 
            USE_DEFAULTS_X, 
            USE_DEFAULTS_Y, 
            USE_DEFAULTS_X + USE_DEFAULTS_WIDTH,
            USE_DEFAULTS_Y + USE_DEFAULTS_HEIGHT
        );
    }

    gSPDisplayList(renderState->dl++, controlsMenu->useDefaults.outline);

    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_ENV_INDEX]);

    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, CONTROLS_X, CONTROLS_Y, CONTROLS_X + CONTROLS_WIDTH, CONTROLS_Y + CONTROLS_HEIGHT);


    gSPDisplayList(renderState->dl++, ui_material_list[DEJAVU_SANS_INDEX]);

    gDPPipeSync(renderState->dl++);
    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WD, SCREEN_HT);
    menuSetRenderColor(renderState, controlsMenu->selectedRow == ControllerActionCount, &gColorBlack, &gColorWhite);
    gSPDisplayList(renderState->dl++, controlsMenu->useDefaults.text);
    gDPPipeSync(renderState->dl++);
    gDPSetEnvColor(renderState->dl++, gColorWhite.r, gColorWhite.g, gColorWhite.b, gColorWhite.a);    
    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, CONTROLS_X, CONTROLS_Y, CONTROLS_X + CONTROLS_WIDTH, CONTROLS_Y + CONTROLS_HEIGHT);

    for (int i = 0; i < ControllerActionCount; ++i) {
        if (controlsMenu->selectedRow == i) {
            gDPPipeSync(renderState->dl++);
            gDPSetEnvColor(renderState->dl++, 0, 0, 0, 255);
        }

        renderStateInlineBranch(renderState, controlsMenu->actionRows[i].actionText);

        if (controlsMenu->selectedRow == i) {
            gDPPipeSync(renderState->dl++);
            gDPSetEnvColor(renderState->dl++, 255, 255, 255, 255);
        }
    }
    for (int i = 0; i < MAX_CONTROLS_SECTIONS; ++i) {
        if (!controlsMenu->headers[i].headerText) {
            break;
        }
        renderStateInlineBranch(renderState, controlsMenu->headers[i].headerText);
    }

    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[BUTTON_ICONS_INDEX]);
    for (int i = 0; i < ControllerActionCount; ++i) {
        if (controlsMenu->selectedRow == i) {
            gDPPipeSync(renderState->dl++);
            gDPSetEnvColor(renderState->dl++, 0, 0, 0, 255);
        }

        renderStateInlineBranch(renderState, controlsMenu->actionRows[i].sourceIcons);

        if (controlsMenu->selectedRow == i) {
            gDPPipeSync(renderState->dl++);
            gDPSetEnvColor(renderState->dl++, 255, 255, 255, 255);
        }
    }
    gSPDisplayList(renderState->dl++, ui_material_revert_list[BUTTON_ICONS_INDEX]);

    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WD, SCREEN_HT);
}