#include "joystick_options.h"

#include "../controls/controller.h"
#include "../font/dejavusans.h"
#include "../audio/soundplayer.h"
#include "../savefile/savefile.h"

#include "../build/assets/materials/ui.h"
#include "../build/src/audio/clips.h"

#define MENU_Y      54
#define MENU_WIDTH  252
#define MENU_HEIGHT 124
#define MENU_X      ((SCREEN_WD - MENU_WIDTH) / 2)

struct MenuElementParams gJoystickMenuParams[] = {
    {
        .type = MenuElementTypeCheckbox,
        .x = MENU_X + 8, 
        .y = MENU_Y + 8,
        .params = {
            .checkbox = {
                .font = &gDejaVuSansFont,
                .messageId = GAMEUI_JOYSTICKINVERTED,
            },
        },
        .selectionIndex = JoystickOptionInvert,
    },
    {
        .type = MenuElementTypeCheckbox,
        .x = MENU_X + 8, 
        .y = MENU_Y + 28,
        .params = {
            .checkbox = {
                .font = &gDejaVuSansFont,
                .messageId = GAMEUI_TANKCONTROLS,
            },
        },
        .selectionIndex = JoystickOptionTankControls,
    },
    {
        .type = MenuElementTypeText,
        .x = MENU_X + 8, 
        .y = MENU_Y + 48,
        .params = {
            .checkbox = {
                .font = &gDejaVuSansFont,
                .messageId = GAMEUI_SENSITIVITY,
            },
        },
        .selectionIndex = JoystickOptionSensitivity,
    },
    {
        .type = MenuElementTypeSlider,
        .x = MENU_X + 8, 
        .y = MENU_Y + 64,
        .params = {
            .slider = {
                .width = 232,
                .numberOfTicks = 9,
                .discrete = 0,
            },
        },
        .selectionIndex = JoystickOptionSensitivity,
    },
    {
        .type = MenuElementTypeText,
        .x = MENU_X + 8, 
        .y = MENU_Y + 84,
        .params = {
            .checkbox = {
                .font = &gDejaVuSansFont,
                .messageId = GAMEUI_ACCELERATION,
            },
        },
        .selectionIndex = JoystickOptionAcceleration,
    },
    {
        .type = MenuElementTypeSlider,
        .x = MENU_X + 8, 
        .y = MENU_Y + 100,
        .params = {
            .slider = {
                .width = 232,
                .numberOfTicks = 9,
                .discrete = 0,
            },
        },
        .selectionIndex = JoystickOptionAcceleration,
    },
    {
        .type = MenuElementTypeText,
        .x = MENU_X + 8, 
        .y = MENU_Y + 120,
        .params = {
            .checkbox = {
                .font = &gDejaVuSansFont,
                .messageId = GAMEUI_DEADZONE,
            },
        },
        .selectionIndex = JoystickOptionDeadzone,
    },
    {
        .type = MenuElementTypeSlider,
        .x = MENU_X + 8, 
        .y = MENU_Y + 136,
        .params = {
            .slider = {
                .width = 232,
                .numberOfTicks = 9,
                .discrete = 0,
            },
        },
        .selectionIndex = JoystickOptionDeadzone,
    },
};

#define INVERT_JOYSTICK_INDEX   0
#define TANK_CONTROLS_INDEX     1
#define SENSITIVITY_INDEX       3
#define ACCELERATION_INDEX      5
#define DEADZONE_INDEX          7

void joystickOptionsAction(void* data, int selection, struct MenuAction* action) {
    switch (selection) {
        case JoystickOptionInvert:
            if (action->state.checkbox.isChecked) {
                gSaveData.controls.flags |= ControlSaveFlagsInvert;
            } else {
                gSaveData.controls.flags &= ~ControlSaveFlagsInvert;
            }
            break;
        case JoystickOptionTankControls:
            if (action->state.checkbox.isChecked) {
                gSaveData.controls.flags |= ControlSaveTankControls;
            } else {
                gSaveData.controls.flags &= ~ControlSaveTankControls;
            }
            break;
        case JoystickOptionSensitivity:
            gSaveData.controls.sensitivity = (short)(action->state.fSlider.value * 0xFFFF + 0.5f);
            break;
        case JoystickOptionAcceleration:
            gSaveData.controls.acceleration = (short)(action->state.fSlider.value * 0xFFFF + 0.5f);
            break;
        case JoystickOptionDeadzone:
            gSaveData.controls.deadzone = (short)(action->state.fSlider.value * 0xFFFF + 0.5f);
            controllerSetDeadzone(action->state.fSlider.value * MAX_DEADZONE);
            break;
    }
}

void joystickOptionsInit(struct JoystickOptions* joystickOptions) {
    menuBuilderInit(
        &joystickOptions->menuBuilder,
        gJoystickMenuParams,
        sizeof(gJoystickMenuParams) / sizeof(*gJoystickMenuParams),
        JoystickOptionCount,
        joystickOptionsAction,
        joystickOptions
    );

    menuBuilderSetCheckbox(&joystickOptions->menuBuilder.elements[INVERT_JOYSTICK_INDEX], (gSaveData.controls.flags & ControlSaveFlagsInvert) != 0);
    menuBuilderSetCheckbox(&joystickOptions->menuBuilder.elements[TANK_CONTROLS_INDEX], (gSaveData.controls.flags & ControlSaveTankControls) != 0);

    menuBuilderSetFSlider(&joystickOptions->menuBuilder.elements[SENSITIVITY_INDEX], (float)gSaveData.controls.sensitivity / 0xFFFF);
    menuBuilderSetFSlider(&joystickOptions->menuBuilder.elements[ACCELERATION_INDEX], (float)gSaveData.controls.acceleration / 0xFFFF);
    menuBuilderSetFSlider(&joystickOptions->menuBuilder.elements[DEADZONE_INDEX], (float)gSaveData.controls.deadzone / 0xFFFF);
}

void joystickOptionsRebuildText(struct JoystickOptions* joystickOptions) {
    menuBuilderRebuildText(&joystickOptions->menuBuilder);
}

enum InputCapture joystickOptionsUpdate(struct JoystickOptions* joystickOptions) {
    return menuBuilderUpdate(&joystickOptions->menuBuilder);
}

void joystickOptionsRender(struct JoystickOptions* joystickOptions, struct RenderState* renderState, struct GraphicsTask* task) {
    menuBuilderRender(&joystickOptions->menuBuilder, renderState);
}
