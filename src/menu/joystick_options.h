#ifndef __MENU_JOYSTICK_OPTIONS_H__
#define __MENU_JOYSTICK_OPTIONS_H__

#include "./menu.h"
#include "../graphics/graphics.h"

enum JoystickOption {
    JoystickOptionInvert,
    JoystickOptionInvertYaw,
    JoystickOptionTankControls,
    JoystickOptionSensitivity,
    JoystickOptionAcceleration,
    JoystickOptionDeadzone,
    
    JoystickOptionCount,
};

struct JoystickOptions {
    struct MenuCheckbox invertControls;
    struct MenuCheckbox invertControlsYaw;
    struct MenuCheckbox tankControls;
    struct MenuSlider lookSensitivity;
    struct MenuSlider lookAcceleration;
    struct MenuSlider joystickDeadzone;
    Gfx* lookSensitivityText;
    Gfx* lookAccelerationText;
    Gfx* joystickDeadzoneText;
    short selectedItem;
};

void joystickOptionsInit(struct JoystickOptions* joystickOptions);
enum MenuDirection joystickOptionsUpdate(struct JoystickOptions* joystickOptions);
void joystickOptionsRender(struct JoystickOptions* joystickOptions, struct RenderState* renderState, struct GraphicsTask* task);

#endif