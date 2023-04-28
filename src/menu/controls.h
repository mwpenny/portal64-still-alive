#ifndef __MENU_CONTROLS_H__
#define __MENU_CONTROLS_H__

#include "./menu.h"
#include "../controls/controller_actions.h"
#include "../graphics/graphics.h"

#define MAX_SOURCES_PER_ACTION  4
#define MAX_CONTROLS_SECTIONS   4

#include "./menu.h"

#define SOURCE_ICON_COUNT MAX_SOURCES_PER_ACTION * GFX_ENTRIES_PER_IMAGE + GFX_ENTRIES_PER_END_DL

struct ControlsMenuRow {
    Gfx* actionText;
    Gfx sourceIcons[SOURCE_ICON_COUNT];
    short y;
};

struct ControlsMenuHeader {
    Gfx* headerText;
};

struct ControlsMenu {
    Gfx* scrollOutline;
    struct ControlsMenuRow actionRows[ControllerActionCount];

    struct ControlsMenuHeader headers[MAX_CONTROLS_SECTIONS];
    Gfx headerSeparators[MAX_CONTROLS_SECTIONS + GFX_ENTRIES_PER_END_DL];

    struct MenuButton useDefaults;

    short selectedRow;
    short scrollOffset;

    enum ControllerAction waitingForAction;
};

void controlsMenuInit(struct ControlsMenu* controlsMenu);
enum MenuDirection controlsMenuUpdate(struct ControlsMenu* controlsMenu);
void controlsMenuRender(struct ControlsMenu* controlsMenu, struct RenderState* renderState, struct GraphicsTask* task);

#endif