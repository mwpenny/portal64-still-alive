#ifndef __MENU_OPTIONS_MENU_H__
#define __MENU_OPTIONS_MENU_H__

#include "../graphics/graphics.h"
#include "./menu.h"
#include "./tabs.h"
#include "./controls.h"
#include "./audio_options.h"
#include "./joystick_options.h"
#include "./gameplay_options.h"
#include "./video_options.h"

enum OptionsMenuTabs {
    OptionsMenuTabsControlMapping,
    OptionsMenuTabsControlJoystick,
    OptionsMenuTabsAudio,
    OptionsMenuTabsVideo,
    OptionsMenuTabsGameplay,

    OptionsMenuTabsCount,
};

struct OptionsMenu {
    Gfx* menuOutline;
    Gfx* optionsText;

    struct Tabs tabs;

    struct ControlsMenu controlsMenu;
    struct JoystickOptions joystickOptions;
    struct AudioOptions audioOptions;
    struct VideoOptions videoOptions;
    struct GameplayOptions gameplayOptions;
};

void optionsMenuInit(struct OptionsMenu* options);
void optionsMenuRebuildText(struct OptionsMenu* options);
enum InputCapture optionsMenuUpdate(struct OptionsMenu* options);
void optionsMenuRender(struct OptionsMenu* options, struct RenderState* renderState, struct GraphicsTask* task);

#endif