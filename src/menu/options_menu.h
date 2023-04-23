#ifndef __MENU_OPTIONS_MENU_H__
#define __MENU_OPTIONS_MENU_H__

#include "../graphics/graphics.h"
#include "./menu.h"
#include "./menu_state.h"

struct OptionsMenu {
    Gfx* menuOutline;
    Gfx* optionsText;
};

void optionsMenuInit(struct OptionsMenu* options);
enum MainMenuState optionsMenuUpdate(struct OptionsMenu* options);
void optionsMenuRender(struct OptionsMenu* options, struct RenderState* renderState, struct GraphicsTask* task);

#endif