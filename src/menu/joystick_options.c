#include "joystick_options.h"

#include "../controls/controller.h"
#include "../font/dejavusans.h"

#include "../build/assets/materials/ui.h"

#define JOYSTICK_Y      54
#define JOYSTICK_WIDTH  252
#define JOYSTICK_HEIGHT 124

#define JOYSTICK_X      ((SCREEN_WD - JOYSTICK_WIDTH) / 2)

void joystickOptionsInit(struct JoystickOptions* joystickOptions) {
    joystickOptions->selectedItem = 0;

    joystickOptions->invertControls = menuBuildCheckbox(&gDejaVuSansFont, "Invert Camera", JOYSTICK_X + 8, JOYSTICK_Y + 8);
    joystickOptions->lookSensitivity = menuBuildSlider(JOYSTICK_X + 8, JOYSTICK_Y + 20, 80, 8, 13);
}

enum MenuDirection joystickOptionsUpdate(struct JoystickOptions* joystickOptions) {
    int controllerDir = controllerGetDirectionDown(0);

    if (controllerGetButtonDown(0, B_BUTTON)) {
        return MenuDirectionUp;
    }

    if (controllerDir & ControllerDirectionLeft) {
        return MenuDirectionLeft;
    }

    if (controllerDir & ControllerDirectionRight) {
        return MenuDirectionRight;
    }

    return MenuDirectionStay;
}

void joystickOptionsRender(struct JoystickOptions* joystickOptions, struct RenderState* renderState, struct GraphicsTask* task) {
    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_ENV_INDEX]);
    
    gSPDisplayList(renderState->dl++, joystickOptions->invertControls.outline);
    gSPDisplayList(renderState->dl++, joystickOptions->lookSensitivity.back);

    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_ENV_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[DEJAVU_SANS_INDEX]);

    gSPDisplayList(renderState->dl++, joystickOptions->invertControls.text);

    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_INDEX]);
}