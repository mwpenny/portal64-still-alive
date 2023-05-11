#ifndef __MENU_JOYSTICK_OPTIONS_H__
#define __MENU_JOYSTICK_OPTIONS_H__

#include "./menu.h"
#include "../graphics/graphics.h"

struct JoystickOptions {
    struct MenuCheckbox invertControls;
    struct MenuSlider lookSensitivity;
    short selectedItem;
};

void joystickOptionsInit(struct JoystickOptions* joystickOptions);
enum MenuDirection joystickOptionsUpdate(struct JoystickOptions* joystickOptions);
void joystickOptionsRender(struct JoystickOptions* joystickOptions, struct RenderState* renderState, struct GraphicsTask* task);

#endif