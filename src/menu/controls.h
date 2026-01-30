#ifndef __MENU_CONTROLS_H__
#define __MENU_CONTROLS_H__

#include "controls/controller_actions.h"
#include "font/font.h"
#include "graphics/graphics.h"
#include "menu.h"
#include "scene/hud.h"

#define MAX_CONTROLS_SECTIONS             4
#define MAX_SOURCES_PER_CONTROLLER_ACTION 4
#define SOURCE_ICON_COUNT                 ((MAX_SOURCES_PER_CONTROLLER_ACTION * GFX_ENTRIES_PER_IMAGE) + GFX_ENTRIES_PER_END_DL)

struct ControlsMenuHeader {
    struct PrerenderedText* headerText;
};

struct ControlsMenuRow {
    struct PrerenderedText* actionText;
    Gfx sourceInputIcons[SOURCE_ICON_COUNT];
    Gfx sourceControllerIndexIcons[SOURCE_ICON_COUNT];
    short y;
};

struct ControlsMenu {
    Gfx* scrollOutline;
    Gfx headerSeparators[MAX_CONTROLS_SECTIONS + GFX_ENTRIES_PER_END_DL];

    struct ControlsMenuHeader headers[MAX_CONTROLS_SECTIONS];
    struct ControlsMenuRow actionRows[ControllerActionCount];
    struct MenuButton useDefaults;

    short selectedRow;
    short scrollOffset;
    enum ControllerAction waitingForAction;
};

void controlsMenuInit(struct ControlsMenu* controlsMenu);
void controlsMenuRebuildText(struct ControlsMenu* controlsMenu);
enum InputCapture controlsMenuUpdate(struct ControlsMenu* controlsMenu);
void controlsMenuRender(struct ControlsMenu* controlsMenu, struct RenderState* renderState, struct GraphicsTask* task);

void controlsRenderPrompt(enum ControllerAction action, char* message, float opacity, struct RenderState* renderState);
void controlsRenderInputIcon(enum ControllerActionInput input, int x, int y, struct RenderState* renderState);

#endif
