#include "controls.h"

#include "audio/soundplayer.h"
#include "font/dejavu_sans.h"
#include "font/font.h"
#include "strings/translations.h"
#include "system/screen.h"
#include "system/controller.h"
#include "util/memory.h"

#include "codegen/assets/materials/ui.h"

#include "codegen/assets/audio/clips.h"
#include "codegen/assets/strings/strings.h"

#define CONTROLS_WIDTH      252
#define CONTROLS_HEIGHT     124
#define CONTROLS_X          ((SCREEN_WD - CONTROLS_WIDTH) / 2)
#define CONTROLS_Y          54

#define OUTLINE_THICKNESS   1
#define CONTROLS_WD_INNER   (CONTROLS_WIDTH  - OUTLINE_THICKNESS)
#define CONTROLS_HT_INNER   (CONTROLS_HEIGHT - OUTLINE_THICKNESS)
#define CONTROLS_X_INNER    (CONTROLS_X      + OUTLINE_THICKNESS)
#define CONTROLS_Y_INNER    (CONTROLS_Y      + OUTLINE_THICKNESS)

#define HEADER_PADDING_X    2
#define HEADER_PADDING_Y    4
#define HEADER_HEIGHT       14

#define SEPARATOR_PADDING_X 8
#define SEPARATOR_PADDING_Y 3
#define SEPARATOR_THICKNESS 1

#define ROW_PADDING_X       8
#define ROW_PADDING_Y       2
#define ROW_TEXT_MAX_WIDTH  190

#define USE_DEFAULTS_X      286
#define USE_DEFAULTS_Y      186
#define USE_DEFAULTS_HEIGHT 16

#define PROMPT_MARGIN_X     17
#define PROMPT_MARGIN_Y     72
#define PROMPT_HEIGHT       24
#define PROMPT_PADDING      6

struct ControllerIcon {
    char x, y;
    char w, h;
};

struct ActionSourceIcon {
    struct ControllerIcon* inputIcon;
    struct ControllerIcon* controllerIndexIcon;
};

enum ControllerButtonIcon {
    ControllerButtonIconA,
    ControllerButtonIconB,
    ControllerButtonIconS,

    ControllerButtonIconCU,
    ControllerButtonIconCR,
    ControllerButtonIconCD,
    ControllerButtonIconCL,

    ControllerButtonIconDU,
    ControllerButtonIconDR,
    ControllerButtonIconDD,
    ControllerButtonIconDL,

    ControllerButtonIconZ,
    ControllerButtonIconR,
    ControllerButtonIconL,
};

enum ControllerDirectionIcon {
    ControllerDirectionIconC,
    ControllerDirectionIconD,
    ControllerDirectionIconJ,
};

static struct ControllerIcon sControllerButtonIcons[] = {
    [ControllerButtonIconA]    = {0, 0, 12, 12},
    [ControllerButtonIconB]    = {12, 0, 12, 12},
    [ControllerButtonIconS]    = {24, 0, 12, 12},

    [ControllerButtonIconCU]   = {0, 24, 12, 12},
    [ControllerButtonIconCR]   = {12, 24, 12, 12},
    [ControllerButtonIconCD]   = {24, 24, 12, 12},
    [ControllerButtonIconCL]   = {36, 24, 12, 12},

    [ControllerButtonIconDU]   = {0, 36, 12, 12},
    [ControllerButtonIconDR]   = {12, 36, 12, 12},
    [ControllerButtonIconDD]   = {24, 36, 12, 12},
    [ControllerButtonIconDL]   = {36, 36, 12, 12},

    [ControllerButtonIconZ]    = {0, 48, 12, 12},
    [ControllerButtonIconR]    = {12, 48, 12, 12},
    [ControllerButtonIconL]    = {24, 48, 12, 12},
};

static struct ControllerIcon sControllerDirectionIcons[] = {
    [ControllerDirectionIconC] = {0, 12, 14, 12},
    [ControllerDirectionIconD] = {14, 12, 12, 12},
    [ControllerDirectionIconJ] = {26, 12, 15, 12},
};

static struct ControllerIcon sControllerIndexIcons[] = {
    { 54, 0, 5, 7 },
    { 59, 0, 5, 7 },
};

static uint8_t sControllerActionInputToButtonIcon[] = {
    [ControllerActionInputAButton]      = ControllerButtonIconA,
    [ControllerActionInputBButton]      = ControllerButtonIconB,
    [ControllerActionInputStartButton]  = ControllerButtonIconS,

    [ControllerActionInputCUpButton]    = ControllerButtonIconCU,
    [ControllerActionInputCRightButton] = ControllerButtonIconCR,
    [ControllerActionInputCDownButton]  = ControllerButtonIconCD,
    [ControllerActionInputCLeftButton]  = ControllerButtonIconCL,

    [ControllerActionInputDUpButton]    = ControllerButtonIconDU,
    [ControllerActionInputDRightButton] = ControllerButtonIconDR,
    [ControllerActionInputDDownButton]  = ControllerButtonIconDD,
    [ControllerActionInputDLeftButton]  = ControllerButtonIconDL,

    [ControllerActionInputZTrig]        = ControllerButtonIconZ,
    [ControllerActionInputRTrig]        = ControllerButtonIconR,
    [ControllerActionInputLTrig]        = ControllerButtonIconL,
};

static uint8_t sControllerActionInputToDirectionIcon[] = {
    [ControllerActionInputCUpButton]    = ControllerDirectionIconC,
    [ControllerActionInputDUpButton]    = ControllerDirectionIconD,
    [ControllerActionInputJoystick]     = ControllerDirectionIconJ,
};

struct ControlActionDataRow {
    enum StringId nameId;
    enum StringId headerId;
    enum ControllerAction action;
};

struct ControlActionDataRow sControllerDataRows[] = {
    {VALVE_MOVE,                VALVE_MOVEMENT_TITLE,                    ControllerActionMove},
    {VALVE_LOOK,                StringIdNone,                            ControllerActionRotate},
    {VALVE_JUMP,                StringIdNone,                            ControllerActionJump},
    {VALVE_DUCK,                StringIdNone,                            ControllerActionDuck},

    {VALVE_PRIMARY_ATTACK,      VALVE_COMBAT_TITLE,                      ControllerActionOpenPortal0},
    {VALVE_SECONDARY_ATTACK,    StringIdNone,                            ControllerActionOpenPortal1},
    {VALVE_USE_ITEMS,           StringIdNone,                            ControllerActionUseItem},

    {VALVE_PAUSE_GAME,          VALVE_MISCELLANEOUS_TITLE,               ControllerActionPause},

    {VALVE_LOOK_STRAIGHT_AHEAD, VALVE_MISCELLANEOUS_KEYBOARD_KEYS_TITLE, ControllerActionLookForward},
    {VALVE_LOOK_STRAIGHT_BACK,  StringIdNone,                            ControllerActionLookBackward},
    {VALVE_OVERVIEW_ZOOMIN,     StringIdNone,                            ControllerActionZoom},
};

static struct Coloru8 sButtonPromptTextColor = { 232, 206, 80, 255 };

static int controlsGetActionSourceIcons(enum ControllerAction action, struct ActionSourceIcon* sourceIcons) {
    struct ControllerActionSource sources[MAX_SOURCES_PER_CONTROLLER_ACTION];
    int sourceCount = controllerActionSources(action, sources, MAX_SOURCES_PER_CONTROLLER_ACTION);

    uint8_t* iconMapping;
    struct ControllerIcon* icons;

    if (ACTION_IS_DIRECTION(action)) {
        iconMapping = sControllerActionInputToDirectionIcon;
        icons = sControllerDirectionIcons;
    } else {
        iconMapping = sControllerActionInputToButtonIcon;
        icons = sControllerButtonIcons;
    }

    for (int i = 0; i < sourceCount; ++i) {
        struct ControllerActionSource* source = &sources[i];
        struct ActionSourceIcon* sourceIcon = &sourceIcons[i];

        sourceIcon->inputIcon = &icons[iconMapping[source->input]];
        sourceIcon->controllerIndexIcon = &sControllerIndexIcons[source->controllerIndex];
    }

    return sourceCount;
}

static Gfx* controlsRenderIcon(Gfx* dl, struct ControllerIcon* icon, int x, int y) {
    gSPTextureRectangle(
        dl++,
        x << 2, y << 2,
        (x + icon->w) << 2, (y + icon->h) << 2,
        G_TX_RENDERTILE,
        icon->x << 5, icon->y << 5,
        0x400, 0x400
    );

    return dl;
}

static Gfx* controlsRenderActionSourceInputIcons(Gfx* dl, struct ActionSourceIcon* sourceIcons, int sourceCount, int x, int y) {
    for (int i = 0; i < sourceCount; ++i) {
        struct ControllerIcon* inputIcon = sourceIcons[i].inputIcon;

        x -= inputIcon->w;
        dl = controlsRenderIcon(dl, inputIcon, x, y);
    }
    
    return dl;
}

static Gfx* controlsRenderActionSourceControllerIndexIcons(Gfx* dl, struct ActionSourceIcon* sourceIcons, int sourceCount, int x, int y) {
    for (int i = 0; i < sourceCount; ++i) {
        struct ActionSourceIcon* sourceIcon = &sourceIcons[i];

        dl = controlsRenderIcon(
            dl,
            sourceIcon->controllerIndexIcon,
            x - sourceIcon->controllerIndexIcon->w,
            y + sourceIcon->inputIcon->h - sourceIcon->controllerIndexIcon->h
        );

        x -= sourceIcon->inputIcon->w;
    }

    return dl;
}

static int controlsActionSourceIconsWidth(struct ActionSourceIcon* sourceIcons, int sourceCount) {
    int result = 0;

    for (int i = 0; i < sourceCount; ++i) {
        struct ControllerIcon* inputIcon = sourceIcons[i].inputIcon;
        result += inputIcon->w;
    }

    return result;
}

static void controlsMenuLayoutRow(struct ControlsMenuRow* row, struct ControlActionDataRow* data, int x, int y) {
    struct PrerenderedText* copy = prerenderedTextCopy(row->actionText);
    menuFreePrerenderedDeferred(row->actionText);
    row->actionText = copy;
    prerenderedTextRelocate(row->actionText, x + ROW_PADDING_X, y);

    struct ActionSourceIcon sourceIcons[MAX_SOURCES_PER_CONTROLLER_ACTION];
    int sourceCount = controlsGetActionSourceIcons(data->action, sourceIcons);

    x = CONTROLS_X + CONTROLS_WIDTH - ROW_PADDING_X;

    Gfx* dl = controlsRenderActionSourceInputIcons(
        row->sourceInputIcons,
        sourceIcons,
        sourceCount,
        x, y
    );
    gSPEndDisplayList(dl++);

    // Disambiguate between multiple bound controllers if necessary
    dl = row->sourceControllerIndexIcons;
    if (controllerActionUsedControllerCount() > 1) {
        dl = controlsRenderActionSourceControllerIndexIcons(
            dl,
            sourceIcons,
            sourceCount,
            x, y
        );
    }
    gSPEndDisplayList(dl++);

    row->y = y;
}

void controlsMenuLayoutHeader(struct ControlsMenuHeader* header, int x, int y) {
    struct PrerenderedText* copy = prerenderedTextCopy(header->headerText);
    menuFreePrerenderedDeferred(header->headerText);
    header->headerText = copy;
    prerenderedTextRelocate(header->headerText, x, y);
}

static void controlsMenuLayout(struct ControlsMenu* controlsMenu) {
    int y = CONTROLS_Y + controlsMenu->scrollOffset;
    int currentHeader = 0;

    Gfx* headerSeparators = controlsMenu->headerSeparators;

    for (int i = 0; i < ControllerActionCount; ++i) {
        if (sControllerDataRows[i].headerId != StringIdNone && currentHeader < MAX_CONTROLS_SECTIONS) {
            y += HEADER_PADDING_Y;
            controlsMenuLayoutHeader(&controlsMenu->headers[currentHeader], CONTROLS_X + HEADER_PADDING_X, y);
            y += HEADER_HEIGHT;

            if ((y + SEPARATOR_THICKNESS) > CONTROLS_Y_INNER && y < (CONTROLS_Y_INNER + CONTROLS_HT_INNER)) {
                gDPFillRectangle(
                    headerSeparators++,
                    CONTROLS_X + SEPARATOR_PADDING_X,
                    y,
                    CONTROLS_X + CONTROLS_WIDTH - SEPARATOR_PADDING_X,
                    y + SEPARATOR_THICKNESS
                );
            }
            y += SEPARATOR_PADDING_Y;
            ++currentHeader;
        }

        controlsMenuLayoutRow(&controlsMenu->actionRows[i], &sControllerDataRows[i], CONTROLS_X, y);
        
        y += controlsMenu->actionRows[i].actionText->height + ROW_PADDING_Y;
    }

    gSPEndDisplayList(headerSeparators++);
}

static void controlsMenuInitRow(struct ControlsMenuRow* row, struct ControlActionDataRow* data) {
    row->actionText = menuBuildPrerenderedText(&gDejaVuSansFont, translationsGet(data->nameId), 0, 0, ROW_TEXT_MAX_WIDTH);

    Gfx* inputIconsDL = row->sourceInputIcons;
    Gfx* controllerIndexIconsDL = row->sourceControllerIndexIcons;
    for (int i = 0; i < SOURCE_ICON_COUNT; ++i) {
        gSPEndDisplayList(inputIconsDL++);
        gSPEndDisplayList(controllerIndexIconsDL++);
    }
}

static void controlsMenuInitHeader(struct ControlsMenuHeader* header, int message) {
    header->headerText = menuBuildPrerenderedText(&gDejaVuSansFont, translationsGet(message), 0, 0, SCREEN_WD);
}

static void controlsMenuInitText(struct ControlsMenu* controlsMenu) {
    int currentHeader = 0;

    for (int i = 0; i < ControllerActionCount; ++i) {
        if (sControllerDataRows[i].headerId != StringIdNone && currentHeader < MAX_CONTROLS_SECTIONS) {
            controlsMenuInitHeader(&controlsMenu->headers[currentHeader], sControllerDataRows[i].headerId);
            ++currentHeader;
        }

        controlsMenuInitRow(&controlsMenu->actionRows[i], &sControllerDataRows[i]);
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

    controlsMenuLayout(controlsMenu);

    controlsMenu->useDefaults = menuBuildButton(
        &gDejaVuSansFont, 
        translationsGet(GAMEUI_USEDEFAULTS),
        USE_DEFAULTS_X, USE_DEFAULTS_Y,
        USE_DEFAULTS_HEIGHT,
        1
    );

    controlsMenu->scrollOutline = menuBuildOutline(
        CONTROLS_X,
        CONTROLS_Y,
        CONTROLS_WIDTH,
        CONTROLS_HEIGHT,
        1
    );
}

void controlsMenuRebuildText(struct ControlsMenu* controlsMenu) {
    for (int i = 0; i < ControllerActionCount; ++i) {
        prerenderedTextFree(controlsMenu->actionRows[i].actionText);
    }

    for (int i = 0; i < MAX_CONTROLS_SECTIONS; ++i) {
        prerenderedTextFree(controlsMenu->headers[i].headerText);
    }

    controlsMenuInitText(controlsMenu);
    controlsMenuLayout(controlsMenu);
    menuRebuildButtonText(
        &controlsMenu->useDefaults,
        &gDejaVuSansFont,
        translationsGet(GAMEUI_USEDEFAULTS),
        1
    );
}

enum InputCapture controlsMenuUpdate(struct ControlsMenu* controlsMenu) {
    if (controlsMenu->waitingForAction != ControllerActionNone) {
        struct ControllerActionSource source;

        if (controllerActionReadAnySource(&source)) {
            if (controllerActionSetSource(controlsMenu->waitingForAction, &source, MAX_SOURCES_PER_CONTROLLER_ACTION)) {
                controlsMenuLayout(controlsMenu);
                soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);
            } else {
                soundPlayerPlay(SOUNDS_WPN_DENYSELECT, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);
            }

            controlsMenu->waitingForAction = ControllerActionNone;
        }

        return InputCaptureGrab;
    }

    int controllerDir = controllerGetDirectionDown(0);
    if (controllerDir & ControllerDirectionDown) {
        controlsMenu->selectedRow = controlsMenu->selectedRow + 1;

        if (controlsMenu->selectedRow == ControllerActionCount + 1) {
            controlsMenu->selectedRow = 0;
        }
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);
    }

    if (controllerDir & ControllerDirectionUp) {
        controlsMenu->selectedRow = controlsMenu->selectedRow - 1;

        if (controlsMenu->selectedRow < 0) {
            controlsMenu->selectedRow = ControllerActionCount;
        }
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);
    }

    if (controlsMenu->selectedRow >= 0 && controlsMenu->selectedRow < ControllerActionCount) {
        struct ControlsMenuRow* selectedAction = &controlsMenu->actionRows[controlsMenu->selectedRow];
        int newScroll = controlsMenu->scrollOffset;
        int topY = selectedAction->y;
        int bottomY = topY + selectedAction->actionText->height + ROW_PADDING_Y + OUTLINE_THICKNESS;

        if (sControllerDataRows[controlsMenu->selectedRow].headerId != StringIdNone) {
            topY -= HEADER_PADDING_Y + HEADER_HEIGHT + SEPARATOR_PADDING_Y;
        } else {
            topY -= ROW_PADDING_Y + OUTLINE_THICKNESS;
        }

        if (topY < CONTROLS_Y) {
            newScroll += CONTROLS_Y - topY;
        } else if (bottomY > (CONTROLS_Y + CONTROLS_HEIGHT)) {
            newScroll += CONTROLS_Y + CONTROLS_HEIGHT - bottomY;
        }

        if (newScroll != controlsMenu->scrollOffset) {
            controlsMenu->scrollOffset = newScroll;
            controlsMenuLayout(controlsMenu);
        }
    }

    if (controllerGetButtonsDown(0, ControllerButtonB)) {
        return InputCaptureExit;
    }

    if (controllerGetButtonsDown(0, ControllerButtonA)) {
        if (controlsMenu->selectedRow >= 0 && controlsMenu->selectedRow < ControllerActionCount) {
            controlsMenu->waitingForAction = sControllerDataRows[controlsMenu->selectedRow].action;
        } else if (controlsMenu->selectedRow == ControllerActionCount) {
            controllerActionSetDefaultSources();
            controlsMenuLayout(controlsMenu);
        }

        soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);
    }

    return InputCapturePass;
}

void controlsMenuRender(struct ControlsMenu* controlsMenu, struct RenderState* renderState, struct GraphicsTask* task) {
    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);
    gDPFillRectangle(renderState->dl++, CONTROLS_X, CONTROLS_Y, CONTROLS_X + CONTROLS_WIDTH, CONTROLS_Y + CONTROLS_HEIGHT);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);

    // Outlines
    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_ENV_INDEX]);
    gSPDisplayList(renderState->dl++, controlsMenu->scrollOutline);
    gDPPipeSync(renderState->dl++);
    gDPSetEnvColor(renderState->dl++, gColorBlack.r, gColorBlack.g, gColorBlack.b, gColorBlack.a);
    renderStateAppendDL(renderState, controlsMenu->headerSeparators);

    // Selection indicator
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
            CONTROLS_X + ROW_PADDING_X,
            selectedAction->y,
            CONTROLS_X + CONTROLS_WIDTH - ROW_PADDING_X,
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

    // Text
    gSPDisplayList(renderState->dl++, ui_material_list[DEJAVU_SANS_0_INDEX]);

    gDPPipeSync(renderState->dl++);
    struct PrerenderedTextBatch* batch = prerenderedBatchStart();
    prerenderedBatchAdd(batch, controlsMenu->useDefaults.text, controlsMenu->selectedRow == ControllerActionCount ? &gColorBlack : &gColorWhite);
    renderState->dl = prerenderedBatchFinish(batch, gDejaVuSansImages, renderState->dl);

    gDPSetScissor(
        renderState->dl++,
        G_SC_NON_INTERLACE,
        CONTROLS_X_INNER,
        CONTROLS_Y_INNER,
        CONTROLS_X_INNER + CONTROLS_WD_INNER,
        CONTROLS_Y_INNER + CONTROLS_HT_INNER
    );

    batch = prerenderedBatchStart();

    for (int i = 0; i < ControllerActionCount; ++i) {
        prerenderedBatchAdd(batch, controlsMenu->actionRows[i].actionText, controlsMenu->selectedRow == i ? &gColorBlack : &gColorWhite);
    }
    for (int i = 0; i < MAX_CONTROLS_SECTIONS; ++i) {
        if (controlsMenu->headers[i].headerText == NULL) {
            break;
        }

        prerenderedBatchAdd(batch, controlsMenu->headers[i].headerText, &gColorWhite);
    }

    renderState->dl = prerenderedBatchFinish(batch, gDejaVuSansImages, renderState->dl);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_0_INDEX]);

    // Input icons
    gSPDisplayList(renderState->dl++, ui_material_list[BUTTON_ICONS_INDEX]);
    for (int i = 0; i < ControllerActionCount; ++i) {
        if (controlsMenu->selectedRow == i) {
            gDPPipeSync(renderState->dl++);
            gDPSetEnvColor(renderState->dl++, gColorBlack.r, gColorBlack.g, gColorBlack.b, gColorBlack.a);

            renderStateAppendDL(renderState, controlsMenu->actionRows[i].sourceInputIcons);

            gDPPipeSync(renderState->dl++);
            gDPSetEnvColor(renderState->dl++, gColorWhite.r, gColorWhite.g, gColorWhite.b, gColorWhite.a);
        } else {
            renderStateAppendDL(renderState, controlsMenu->actionRows[i].sourceInputIcons);
        }
    }

    gDPPipeSync(renderState->dl++);
    gDPSetEnvColor(renderState->dl++, gColorBlack.r, gColorBlack.g, gColorBlack.b, gColorBlack.a);
    for (int i = 0; i < ControllerActionCount; ++i) {
        if (controlsMenu->selectedRow == i) {
            gDPPipeSync(renderState->dl++);
            gDPSetEnvColor(renderState->dl++, gColorWhite.r, gColorWhite.g, gColorWhite.b, gColorWhite.a);

            renderStateAppendDL(renderState, controlsMenu->actionRows[i].sourceControllerIndexIcons);

            gDPPipeSync(renderState->dl++);
            gDPSetEnvColor(renderState->dl++, gColorBlack.r, gColorBlack.g, gColorBlack.b, gColorBlack.a);
        } else {
            renderStateAppendDL(renderState, controlsMenu->actionRows[i].sourceControllerIndexIcons);
        }
    }

    gSPDisplayList(renderState->dl++, ui_material_revert_list[BUTTON_ICONS_INDEX]);

    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WD, SCREEN_HT);
}

void controlsRenderPrompt(enum ControllerAction action, char* message, float opacity, struct RenderState* renderState) {
    if (message == NULL || *message == '\0') {
        return;
    }
    
    struct FontRenderer* fontRender = stackMalloc(sizeof(struct FontRenderer));
    fontRendererLayout(fontRender, &gDejaVuSansFont, message, SCREEN_WD - (PROMPT_MARGIN_X + (PROMPT_PADDING * 2)));
    
    struct ActionSourceIcon sourceIcons[MAX_SOURCES_PER_CONTROLLER_ACTION];
    int sourceCount = controlsGetActionSourceIcons(action, sourceIcons);

    int iconsWidth = controlsActionSourceIconsWidth(sourceIcons, sourceCount);
    int opacityAsInt = (int)(255.0f * opacity);

    int textPositionX = (SCREEN_WD - PROMPT_MARGIN_X - PROMPT_PADDING) - fontRender->width;
    int textPositionY = (SCREEN_HT - PROMPT_MARGIN_Y - PROMPT_PADDING) - fontRender->height;
    
    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);
    gDPSetEnvColor(renderState->dl++, 0, 0, 0, opacityAsInt / 3);
    gDPFillRectangle(
        renderState->dl++,
        textPositionX - iconsWidth - (PROMPT_PADDING * 2),
        textPositionY - PROMPT_PADDING,
        SCREEN_WD - PROMPT_MARGIN_X,
        SCREEN_HT - PROMPT_MARGIN_Y
    );
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);

    struct Coloru8 textColor = sButtonPromptTextColor;
    textColor.a = opacityAsInt;
        
    renderState->dl = fontRendererBuildGfx(fontRender, gDejaVuSansImages, textPositionX, textPositionY, &textColor, renderState->dl);
    
    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_0_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[BUTTON_ICONS_INDEX]);
    gDPSetEnvColor(renderState->dl++, textColor.r, textColor.g, textColor.b, textColor.a);
    renderState->dl = controlsRenderActionSourceInputIcons(
        renderState->dl,
        sourceIcons,
        sourceCount,
        textPositionX - PROMPT_PADDING,
        textPositionY
    );

    // Disambiguate between multiple bound controllers if necessary
    if (controllerActionUsedControllerCount() > 1) {
        gDPPipeSync(renderState->dl++);
        gDPSetEnvColor(renderState->dl++, gColorWhite.r, gColorWhite.g, gColorWhite.b, textColor.a);

        renderState->dl = controlsRenderActionSourceControllerIndexIcons(
            renderState->dl,
            sourceIcons,
            sourceCount,
            textPositionX - PROMPT_PADDING,
            textPositionY
        );
    }

    gSPDisplayList(renderState->dl++, ui_material_revert_list[BUTTON_ICONS_INDEX]);
    
    stackMallocFree(fontRender);
}

void controlsRenderInputIcon(enum ControllerActionInput input, int x, int y, struct RenderState* renderState) {
    struct ControllerIcon* icon;

    if (input == ControllerActionInputJoystick) {
        icon = &sControllerDirectionIcons[ControllerDirectionIconJ];
    } else {
        icon = &sControllerButtonIcons[sControllerActionInputToButtonIcon[input]];
    }

    renderState->dl = controlsRenderIcon(renderState->dl, icon, x, y);
}
