#include "controls.h"

#include "../font/dejavusans.h"
#include "../font/font.h"
#include "../controls/controller.h"
#include "../audio/soundplayer.h"
#include "../util/memory.h"
#include "./translations.h"

#include "../build/assets/materials/ui.h"

#include "../build/src/audio/clips.h"
#include "../build/src/audio/subtitles.h"

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

#define USE_DEFAULTS_X      286
#define USE_DEFAULTS_Y      186
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
    short nameId;
    short headerId;
    enum ControllerAction action;
};

struct ControlActionDataRow gControllerDataRows[] = {
    {VALVE_MOVE, VALVE_MOVEMENT_TITLE, ControllerActionMove},
    {VALVE_LOOK, -1, ControllerActionRotate},
    {VALVE_JUMP, -1, ControllerActionJump},
    {VALVE_DUCK, -1, ControllerActionDuck},

    {VALVE_PRIMARY_ATTACK, VALVE_COMBAT_TITLE, ControllerActionOpenPortal0},
    {VALVE_SECONDARY_ATTACK, -1, ControllerActionOpenPortal1},
    {VALVE_USE_ITEMS, -1, ControllerActionUseItem},

    {VALVE_PAUSE_GAME, VALVE_MISCELLANEOUS_TITLE, ControllerActionPause},

    {VALVE_LOOK_STRAIGHT_AHEAD, VALVE_MISCELLANEOUS_KEYBOARD_KEYS_TITLE, ControllerActionLookForward},
    {VALVE_LOOK_STRAIGHT_BACK, -1, ControllerActionLookBackward},
};

int controlsMeasureIcons(enum ControllerAction action) {
    struct ControllerSourceWithController sources[MAX_SOURCES_PER_ACTION];

    int sourceCount = controllerSourcesForAction(action, sources, MAX_SOURCES_PER_ACTION);

    char* iconMapping;
    struct ControllerIcon* icons;
    
    if (IS_DIRECTION_ACTION(action)) {
        iconMapping = gControllerActionToDirectionIcon;
        icons = gControllerDirectionIcons;
    } else {
        iconMapping = gControllerActionToButtonIcon;
        icons = gControllerButtonIcons;
    }

    int result = 0;

    for (int i = 0; i < sourceCount; ++i) {
        struct ControllerIcon icon = icons[(int)iconMapping[(int)sources[i].button]];
        result += icon.w;
    }

    return result;
}

Gfx* controlsRenderActionIcons(Gfx* dl, enum ControllerAction action, int x, int y) {
    struct ControllerSourceWithController sources[MAX_SOURCES_PER_ACTION];

    int sourceCount = controllerSourcesForAction(action, sources, MAX_SOURCES_PER_ACTION);

    char* iconMapping;
    struct ControllerIcon* icons;
    
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
    
    return dl;
}

void controlsRenderButtonIcon(enum ControllerActionSource source, int x, int y, struct RenderState* renderState) {
    struct ControllerIcon icon;

    source = controllerSourceMapAction(source);
    if (!IS_VALID_SOURCE(source)) {
        return;
    }

    icon = gControllerButtonIcons[(int)gControllerActionToButtonIcon[(int)source]];
    gSPTextureRectangle(
        renderState->dl++,
        x << 2, y << 2,
        (x + icon.w) << 2, (y + icon.h) << 2,
        G_TX_RENDERTILE,
        icon.x << 5, icon.y << 5,
        0x400, 0x400
    );
}

void controlsLayoutRow(struct ControlsMenuRow* row, struct ControlActionDataRow* data, int x, int y) {
    struct PrerenderedText* copy = prerenderedTextCopy(row->actionText);
    menuFreePrerenderedDeferred(row->actionText);
    row->actionText = copy;
    prerenderedTextRelocate(row->actionText, x + ROW_PADDING, y);
    Gfx* dl = controlsRenderActionIcons(row->sourceIcons, data->action, CONTROLS_X + CONTROLS_WIDTH - ROW_PADDING * 2, y);
    gSPEndDisplayList(dl++);
    row->y = y;
}

void controlsLayoutHeader(struct ControlsMenuHeader* header, int x, int y) {
    struct PrerenderedText* copy = prerenderedTextCopy(header->headerText);
    menuFreePrerenderedDeferred(header->headerText);
    header->headerText = copy;
    prerenderedTextRelocate(header->headerText, x, y);
}

void controlsInitRow(struct ControlsMenuRow* row, struct ControlActionDataRow* data) {
    row->actionText = menuBuildPrerenderedText(&gDejaVuSansFont, translationsGet(data->nameId), 0, 0, 200);

    Gfx* dl = row->sourceIcons;
    for (int i = 0; i < SOURCE_ICON_COUNT; ++i) {
        gSPEndDisplayList(dl++);
    }
}

void controlsInitHeader(struct ControlsMenuHeader* header, int message) {
    header->headerText = menuBuildPrerenderedText(&gDejaVuSansFont, translationsGet(message), 0, 0, SCREEN_WD);
}

void controlsLayout(struct ControlsMenu* controlsMenu) {
    int y = CONTROLS_Y + controlsMenu->scrollOffset;
    int currentHeader = 0;

    Gfx* headerSeparators = controlsMenu->headerSeparators;

    for (int i = 0; i < ControllerActionCount; ++i) {
        if (gControllerDataRows[i].headerId != -1 && currentHeader < MAX_CONTROLS_SECTIONS) {
            y += TOP_PADDING;
            controlsLayoutHeader(&controlsMenu->headers[currentHeader], CONTROLS_X + 2, y);
            y += CONTROL_ROW_HEIGHT;

            if (y > CONTROLS_Y + 1 && y < CONTROLS_Y + CONTROLS_HEIGHT - 1) {
                gDPFillRectangle(headerSeparators++, CONTROLS_X + SEPARATOR_PADDING, y, CONTROLS_X + CONTROLS_WIDTH - SEPARATOR_PADDING * 2, y + 1);
            }
            y += SEPARATOR_SPACE;
            ++currentHeader;
        }

        controlsLayoutRow(&controlsMenu->actionRows[i], &gControllerDataRows[i], CONTROLS_X, y);
        
        y += controlsMenu->actionRows[i].actionText->height + 2;
    }

    gSPEndDisplayList(headerSeparators++);
}

void controlsMenuInitText(struct ControlsMenu* controlsMenu) {
    int currentHeader = 0;

    for (int i = 0; i < ControllerActionCount; ++i) {
        controlsInitRow(&controlsMenu->actionRows[i], &gControllerDataRows[i]);

        if (gControllerDataRows[i].headerId != -1 && currentHeader < MAX_CONTROLS_SECTIONS) {
            controlsInitHeader(&controlsMenu->headers[currentHeader], gControllerDataRows[i].headerId);
            ++currentHeader;
        }
    }

    for (; currentHeader < MAX_CONTROLS_SECTIONS; ++currentHeader) {
        controlsMenu->headers[currentHeader].headerText = NULL;
    }
}

void controlsMenuInit(struct ControlsMenu* controlsMenu) {
    controlsMenuInitText(controlsMenu);

    controlsMenu->selectedRow = 0;
    controlsMenu->scrollOffset = 0;
    controlsMenu->waitingForAction = ControllerActionNone;

    controlsLayout(controlsMenu);

    controlsMenu->useDefaults = menuBuildButton(
        &gDejaVuSansFont, 
        translationsGet(GAMEUI_USEDEFAULTS),
        USE_DEFAULTS_X, USE_DEFAULTS_Y,
        USE_DEFAULTS_HEIGHT,
        1
    );

    controlsMenu->scrollOutline = menuBuildOutline(CONTROLS_X, CONTROLS_Y, CONTROLS_WIDTH, CONTROLS_HEIGHT, 1);
}

void controlsRebuildtext(struct ControlsMenu* controlsMenu) {
    for (int i = 0; i < ControllerActionCount; ++i) {
        prerenderedTextFree(controlsMenu->actionRows[i].actionText);
    }

    for (int i = 0; i < MAX_CONTROLS_SECTIONS; ++i) {
        prerenderedTextFree(controlsMenu->headers[i].headerText);
    }

    controlsMenuInitText(controlsMenu);
    controlsLayout(controlsMenu);
    menuRebuildButtonText(&controlsMenu->useDefaults, &gDejaVuSansFont, translationsGet(GAMEUI_USEDEFAULTS), 1);
}

enum InputCapture controlsMenuUpdate(struct ControlsMenu* controlsMenu) {
    if (controlsMenu->waitingForAction != ControllerActionNone) {
        struct ControllerSourceWithController source = controllerReadAnySource();

        if (IS_VALID_SOURCE(source.button)) {
            controllerSetSource(controlsMenu->waitingForAction, source.button, source.controller);
            controlsMenu->waitingForAction = ControllerActionNone;

            controlsLayout(controlsMenu);

            soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
        }

        return InputCaptureGrab;
    }

    int controllerDir = controllerGetDirectionDown(0);
    if (controllerDir & ControllerDirectionDown) {
        controlsMenu->selectedRow = controlsMenu->selectedRow + 1;

        if (controlsMenu->selectedRow == ControllerActionCount + 1) {
            controlsMenu->selectedRow = 0;
        }
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
    }

    if (controllerDir & ControllerDirectionUp) {
        controlsMenu->selectedRow = controlsMenu->selectedRow - 1;

        if (controlsMenu->selectedRow < 0) {
            controlsMenu->selectedRow = ControllerActionCount;
        }
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
    }

    if (controlsMenu->selectedRow >= 0 && controlsMenu->selectedRow < ControllerActionCount) {
        struct ControlsMenuRow* selectedAction = &controlsMenu->actionRows[controlsMenu->selectedRow];
        int newScroll = controlsMenu->scrollOffset;
        int topY = selectedAction->y;
        int bottomY = topY + selectedAction->actionText->height + 2 + TOP_PADDING;

        if (gControllerDataRows[controlsMenu->selectedRow].headerId != -1) {
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

        soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
    }

    return InputCapturePass;
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
            selectedAction->y + selectedAction->actionText->height
        );
    }

    if (controlsMenu->selectedRow == ControllerActionCount) {
        gDPPipeSync(renderState->dl++);
        gDPSetEnvColor(renderState->dl++, gSelectionOrange.r, gSelectionOrange.g, gSelectionOrange.b, gSelectionOrange.a);
        gDPFillRectangle(
            renderState->dl++, 
            controlsMenu->useDefaults.x, 
            USE_DEFAULTS_Y, 
            controlsMenu->useDefaults.x + controlsMenu->useDefaults.w,
            USE_DEFAULTS_Y + USE_DEFAULTS_HEIGHT
        );
    }

    gSPDisplayList(renderState->dl++, controlsMenu->useDefaults.outline);

    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_ENV_INDEX]);

    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, CONTROLS_X, CONTROLS_Y, CONTROLS_X + CONTROLS_WIDTH, CONTROLS_Y + CONTROLS_HEIGHT);


    gSPDisplayList(renderState->dl++, ui_material_list[DEJAVU_SANS_0_INDEX]);

    gDPPipeSync(renderState->dl++);
    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WD, SCREEN_HT);
    struct PrerenderedTextBatch* batch = prerenderedBatchStart();
    prerenderedBatchAdd(batch, controlsMenu->useDefaults.text, controlsMenu->selectedRow == ControllerActionCount ? &gColorBlack : &gColorWhite);
    renderState->dl = prerenderedBatchFinish(batch, gDejaVuSansImages, renderState->dl);

    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, CONTROLS_X, CONTROLS_Y, CONTROLS_X + CONTROLS_WIDTH, CONTROLS_Y + CONTROLS_HEIGHT);

    batch = prerenderedBatchStart();

    for (int i = 0; i < ControllerActionCount; ++i) {
        prerenderedBatchAdd(batch, controlsMenu->actionRows[i].actionText, controlsMenu->selectedRow == i ? &gColorBlack : &gColorWhite);
    }
    for (int i = 0; i < MAX_CONTROLS_SECTIONS; ++i) {
        if (!controlsMenu->headers[i].headerText) {
            break;
        }
        prerenderedBatchAdd(batch, controlsMenu->headers[i].headerText, &gColorWhite);
    }

    renderState->dl = prerenderedBatchFinish(batch, gDejaVuSansImages, renderState->dl);

    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_0_INDEX]);

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

#define CONTROL_PROMPT_RIGHT_MARGIN     20
#define CONTROL_PROMPT_BOTTOM_MARGIN    50
#define CONTROL_PROMPT_HEIGHT           24
#define CONTROL_PROMPT_PADDING          6

#define SUBTITLE_SIDE_MARGIN     17
#define SUBTITLE_BOTTOM_MARGIN    11
#define SUBTITLE_PADDING   5


void controlsRenderPrompt(enum ControllerAction action, char* message, float opacity, struct RenderState* renderState) {
    if (message == NULL || (message != NULL && message[0] == '\0'))
        return;
    
    struct FontRenderer* fontRender = stackMalloc(sizeof(struct FontRenderer));
    fontRendererLayout(fontRender, &gDejaVuSansFont, message, SCREEN_WD - (CONTROL_PROMPT_RIGHT_MARGIN + (CONTROL_PROMPT_PADDING * 2)));
    
    int iconsWidth = controlsMeasureIcons(action);

    int opacityAsInt = (int)(255 * opacity);

    if (opacityAsInt > 255) {
        opacityAsInt = 255;
    } else if (opacityAsInt < 0) {
        opacityAsInt = 0;
    }

    int textPositionX = (SCREEN_WD - CONTROL_PROMPT_RIGHT_MARGIN - CONTROL_PROMPT_PADDING) - fontRender->width;
    int textPositionY = (SCREEN_HT - CONTROL_PROMPT_BOTTOM_MARGIN - CONTROL_PROMPT_PADDING) - fontRender->height;
    
    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);
    gDPSetEnvColor(renderState->dl++, 0, 0, 0, opacityAsInt / 3);
    gDPFillRectangle(
        renderState->dl++, 
        textPositionX - iconsWidth - CONTROL_PROMPT_PADDING * 2,
        textPositionY - CONTROL_PROMPT_PADDING,
        SCREEN_WD - CONTROL_PROMPT_RIGHT_MARGIN, 
        SCREEN_HT - CONTROL_PROMPT_BOTTOM_MARGIN
    );
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);

    struct Coloru8 textColor;
    
    textColor.r = 232;
    textColor.g = 206;
    textColor.b = 80;
    textColor.a = opacityAsInt;
        
    renderState->dl = fontRendererBuildGfx(fontRender, gDejaVuSansImages, textPositionX, textPositionY, &textColor, renderState->dl);
    
    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_0_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[BUTTON_ICONS_INDEX]);
    gDPSetEnvColor(renderState->dl++, 232, 206, 80, opacityAsInt);
    renderState->dl = controlsRenderActionIcons(renderState->dl, action, textPositionX - CONTROL_PROMPT_PADDING, textPositionY);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[BUTTON_ICONS_INDEX]);
    
    stackMallocFree(fontRender);
}

void controlsRenderSubtitle(char* message, float textOpacity, float backgroundOpacity, struct RenderState* renderState, enum SubtitleType subtitleType) {
    if (message == NULL || (message != NULL && message[0] == '\0'))
        return;
    
    struct FontRenderer* fontRender = stackMalloc(sizeof(struct FontRenderer));
    fontRendererLayout(fontRender, &gDejaVuSansFont, message, SCREEN_WD - (SUBTITLE_SIDE_MARGIN + SUBTITLE_PADDING) * 2);

    int textOpacityAsInt = (int)(255 * textOpacity);

    if (textOpacityAsInt > 255) {
        textOpacityAsInt = 255;
    } else if (textOpacityAsInt < 0) {
        textOpacityAsInt = 0;
    }

    int backgroundOpacityAsInt = (int)(255 * backgroundOpacity);

    if (backgroundOpacityAsInt > 255) {
        backgroundOpacityAsInt = 255;
    } else if (backgroundOpacityAsInt < 0) {
        backgroundOpacityAsInt = 0;
    }

    int textPositionX = (SUBTITLE_SIDE_MARGIN + SUBTITLE_PADDING);
    int textPositionY = (SCREEN_HT - SUBTITLE_BOTTOM_MARGIN - SUBTITLE_PADDING) - fontRender->height;

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);
    gDPSetEnvColor(renderState->dl++, 0, 0, 0, backgroundOpacityAsInt);
    gDPFillRectangle(
        renderState->dl++, 
        textPositionX - CONTROL_PROMPT_PADDING,
        textPositionY - CONTROL_PROMPT_PADDING,
        SCREEN_WD - SUBTITLE_SIDE_MARGIN, 
        SCREEN_HT - SUBTITLE_BOTTOM_MARGIN
    );
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);

    struct Coloru8 textColor;

    if (subtitleType == SubtitleTypeCloseCaption) {
        textColor.r = 255;
        textColor.g = 140;
        textColor.b = 155;
    } else if (subtitleType == SubtitleTypeCaption) {
        textColor = gColorWhite;
    }

    textColor.a = textOpacityAsInt;

    renderState->dl = fontRendererBuildGfx(fontRender, gDejaVuSansImages, textPositionX, textPositionY, &textColor, renderState->dl);

    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_0_INDEX]);

    stackMallocFree(fontRender);
}
