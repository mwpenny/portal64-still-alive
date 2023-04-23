#ifndef __MENU_LANDING_MENU_H__
#define __MENU_LANDING_MENU_H__

#include "../graphics/graphics.h"
#include "./menu.h"
#include "./menu_state.h"

struct LandingMenu {
    Gfx* newGameText;
    Gfx* loadGameText;
    Gfx* optionsText;
    short selectedItem;
};

void landingMenuInit(struct LandingMenu* landingMenu);
enum MainMenuState landingMenuUpdate(struct LandingMenu* landingMenu);
void landingMenuRender(struct LandingMenu* landingMenu, struct RenderState* renderState, struct GraphicsTask* task);


#endif