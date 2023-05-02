#ifndef __MENU_LANDING_MENU_H__
#define __MENU_LANDING_MENU_H__

#include "../graphics/graphics.h"
#include "./menu.h"

struct LandingMenuOption {
    char* message;
    int id;
};

struct LandingMenu {
    struct LandingMenuOption* options;
    Gfx** optionText;
    short selectedItem;
    short optionCount;
    short darkenBackground;
};

void landingMenuInit(struct LandingMenu* landingMenu, struct LandingMenuOption* options, int optionCount, int darkenBackground);
struct LandingMenuOption* landingMenuUpdate(struct LandingMenu* landingMenu);
void landingMenuRender(struct LandingMenu* landingMenu, struct RenderState* renderState, struct GraphicsTask* task);


#endif