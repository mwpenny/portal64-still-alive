#ifndef __MENU_GAMEPLAY_OPTIONS_H__
#define __MENU_GAMEPLAY_OPTIONS_H__

#include "./menu.h"
#include "../graphics/graphics.h"
#include "./menu_builder.h"

enum GameplayOption {
    GameplayOptionMovingPortals,
    GameplayOptionPortalFunneling,
    GameplayOptionPortalRenderDepth,
    
    GameplayOptionCount,
};

struct GameplayOptions {
    struct MenuBuilder menuBuilder;
};

void gameplayOptionsInit(struct GameplayOptions* gameplayOptions);
void gameplayOptionsRebuildText(struct GameplayOptions* gameplayOptions);
enum InputCapture gameplayOptionsUpdate(struct GameplayOptions* gameplayOptions);
void gameplayOptionsRender(struct GameplayOptions* gameplayOptions, struct RenderState* renderState, struct GraphicsTask* task);

#endif
