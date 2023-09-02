#ifndef __MENU_JOYSTICK_OPTIONS_H__
#define __MENU_JOYSTICK_OPTIONS_H__

#include "./menu.h"
#include "../graphics/graphics.h"

enum JoystickOption {
    JoystickOptionInvert,
    JoystickOptionTankControls,
    JoystickOptionSensitivity,
    JoystickOptionAcceleration,
    
    JoystickOptionCount,
};

struct JoystickOptions {
    struct MenuCheckbox invertControls;
    struct MenuCheckbox tankControls;
    struct MenuSlider lookSensitivity;
    struct MenuSlider lookAcceleration;
    Gfx* lookSensitivityText;
    Gfx* lookAccelerationText;
    short selectedItem;
};

void joystickOptionsInit(struct JoystickOptions* joystickOptions);
enum MenuDirection joystickOptionsUpdate(struct JoystickOptions* joystickOptions);
void joystickOptionsRender(struct JoystickOptions* joystickOptions, struct RenderState* renderState, struct GraphicsTask* task);

#endif