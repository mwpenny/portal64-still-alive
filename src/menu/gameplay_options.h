#ifndef __MENU_GAMEPLAY_OPTIONS_H__
#define __MENU_GAMEPLAY_OPTIONS_H__

#include "./menu.h"
#include "../graphics/graphics.h"

enum GameplayOption {
    GameplayOptionMovingPortals,
    
    GameplayOptionCount,
};

struct GameplayOptions {
    struct MenuCheckbox movingPortals;
    Gfx* lookSensitivityText;
    Gfx* lookAccelerationText;
    short selectedItem;
};

void gameplayOptionsInit(struct GameplayOptions* gameplayOptions);
enum MenuDirection gameplayOptionsUpdate(struct GameplayOptions* gameplayOptions);
void gameplayOptionsRender(struct GameplayOptions* gameplayOptions, struct RenderState* renderState, struct GraphicsTask* task);

#endif