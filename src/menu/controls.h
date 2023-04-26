#ifndef __MENU_CONTROLS_H__
#define __MENU_CONTROLS_H__

#include "./menu.h"
#include "../controls/controller_actions.h"
#include "../graphics/graphics.h"

#define MAX_SOURCES_PER_ACTION  4

struct ControlsMenuRow {
    Gfx* actionText;
    Gfx sourceIcons[MAX_SOURCES_PER_ACTION * GFX_ENTRIES_PER_IMAGE + GFX_ENTRIES_PER_END_DL];
};

struct ControlsMenu {
    Gfx* scrollOutline;
    struct ControlsMenuRow actionRows[ControllerActionCount];
};

void controlsMenuInit(struct ControlsMenu* controlsMenu);

void controlsMenuRender(struct ControlsMenu* controlsMenu, struct RenderState* renderState, struct GraphicsTask* task);

#endif