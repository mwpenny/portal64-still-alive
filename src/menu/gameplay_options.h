#ifndef __MENU_GAMEPLAY_OPTIONS_H__
#define __MENU_GAMEPLAY_OPTIONS_H__

#include "./menu.h"
#include "../graphics/graphics.h"

enum GameplayOption {
    GameplayOptionMovingPortals,
    GameplayOptionWideScreen,
    GameplayOptionPortalFunneling,
    GameplayOptionPortalRenderDepth,
    
    GameplayOptionCount,
};

struct GameplayOptions {
    struct MenuCheckbox movingPortals;
    struct MenuCheckbox wideScreen;
    struct MenuCheckbox portalFunnel;
    struct MenuSlider portalRenderDepth;
    Gfx* portalRenderDepthText;
    short selectedItem;
    unsigned short render_depth;
};

void gameplayOptionsInit(struct GameplayOptions* gameplayOptions);
enum MenuDirection gameplayOptionsUpdate(struct GameplayOptions* gameplayOptions);
void gameplayOptionsRender(struct GameplayOptions* gameplayOptions, struct RenderState* renderState, struct GraphicsTask* task);

#endif