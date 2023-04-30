#ifndef __MENU_OPTIONS_MENU_H__
#define __MENU_OPTIONS_MENU_H__

#include "../graphics/graphics.h"
#include "./menu.h"
#include "./menu_state.h"
#include "./tabs.h"
#include "./controls.h"
#include "./audio_options.h"

enum OptionsMenuTabs {
    OptionsMenuTabsControls,
    OptionsMenuTabsAudio,

    OptionsMenuTabsCount,
};

struct OptionsMenu {
    Gfx* menuOutline;
    Gfx* optionsText;

    struct Tabs tabs;

    struct ControlsMenu controlsMenu;
    struct AudioOptions audioOptions;
};

void optionsMenuInit(struct OptionsMenu* options);
enum MenuDirection optionsMenuUpdate(struct OptionsMenu* options);
void optionsMenuRender(struct OptionsMenu* options, struct RenderState* renderState, struct GraphicsTask* task);

#endif