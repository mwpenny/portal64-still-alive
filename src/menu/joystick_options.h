#ifndef __MENU_JOYSTICK_OPTIONS_H__
#define __MENU_JOYSTICK_OPTIONS_H__

#include "./menu.h"
#include "../graphics/graphics.h"
#include "./menu_builder.h"

enum JoystickOption {
    JoystickOptionInvert,
    JoystickOptionTankControls,
    JoystickOptionSensitivity,
    JoystickOptionAcceleration,
    JoystickOptionDeadzone,
    
    JoystickOptionCount,
};

struct JoystickOptions {
    struct MenuBuilder menuBuilder;
};

void joystickOptionsInit(struct JoystickOptions* joystickOptions);
void joystickOptionsRebuildText(struct JoystickOptions* joystickOptions);
enum InputCapture joystickOptionsUpdate(struct JoystickOptions* joystickOptions);
void joystickOptionsRender(struct JoystickOptions* joystickOptions, struct RenderState* renderState, struct GraphicsTask* task);

#endif