#ifndef __MENU_LANDING_MENU_H__
#define __MENU_LANDING_MENU_H__

#include "../graphics/graphics.h"
#include "../font/font.h"
#include "./menu.h"

struct LandingMenuOption {
    short messageId;
    short id;
};

struct LandingMenu {
    struct LandingMenuOption* options;
    struct PrerenderedText** optionText;
    short selectedItem;
    short optionCount;
    short darkenBackground;
};

void landingMenuInit(struct LandingMenu* landingMenu, struct LandingMenuOption* options, int optionCount, int darkenBackground);
void landingMenuRebuildText(struct LandingMenu* landingMenu);
struct LandingMenuOption* landingMenuUpdate(struct LandingMenu* landingMenu);
void landingMenuRender(struct LandingMenu* landingMenu, struct RenderState* renderState, struct GraphicsTask* task);
int getCurrentStrideValue(struct LandingMenu* landingMenu);

#endif