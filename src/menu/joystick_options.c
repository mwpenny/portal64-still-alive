#include "joystick_options.h"

#include "../controls/controller.h"
#include "../font/dejavusans.h"
#include "../audio/soundplayer.h"
#include "../savefile/savefile.h"

#include "../build/assets/materials/ui.h"
#include "../build/src/audio/clips.h"

#define JOYSTICK_Y      54
#define JOYSTICK_WIDTH  252
#define JOYSTICK_HEIGHT 124

#define SCROLL_TICKS        9
#define SCROLL_INTERVALS    (SCROLL_TICKS - 1)

#define JOYSTICK_X      ((SCREEN_WD - JOYSTICK_WIDTH) / 2)

void joystickOptionsInit(struct JoystickOptions* joystickOptions) {
    joystickOptions->selectedItem = JoystickOptionInvert;

    joystickOptions->invertControls = menuBuildCheckbox(&gDejaVuSansFont, "Invert Camera", JOYSTICK_X + 8, JOYSTICK_Y + 8);

    joystickOptions->lookSensitivityText = menuBuildText(&gDejaVuSansFont, "Look Sensitivity", JOYSTICK_X + 8, JOYSTICK_Y + 28);
    joystickOptions->lookSensitivity = menuBuildSlider(JOYSTICK_X + 120, JOYSTICK_Y + 28, 120, SCROLL_TICKS);

    joystickOptions->lookAccelerationText = menuBuildText(&gDejaVuSansFont, "Look Acceleration", JOYSTICK_X + 8, JOYSTICK_Y + 48);
    joystickOptions->lookAcceleration = menuBuildSlider(JOYSTICK_X + 120, JOYSTICK_Y + 48, 120, SCROLL_TICKS);

    joystickOptions->invertControls.checked = (gSaveData.controls.flags & ControlSaveFlagsInvert) != 0;
    joystickOptions->lookSensitivity.value = (float)gSaveData.controls.sensitivity / 0xFFFF;
    joystickOptions->lookAcceleration.value = (float)gSaveData.controls.acceleration / 0xFFFF;
}

#define FULL_SCROLL_TIME    2.0f
#define SCROLL_MULTIPLIER   (int)(0xFFFF * FIXED_DELTA_TIME / (80 * FULL_SCROLL_TIME))

#define SCROLL_CHUNK_SIZE   (0x10000 / SCROLL_INTERVALS)

void joystickOptionsHandleSlider(unsigned short* settingValue, float* sliderValue) {
    OSContPad* pad = controllersGetControllerData(0);

    int newValue = (int)*settingValue + pad->stick_x * SCROLL_MULTIPLIER;

    if (controllerGetButtonDown(0, A_BUTTON | R_JPAD)) {
        if (newValue >= 0xFFFF && controllerGetButtonDown(0, A_BUTTON)) {
            newValue = 0;
        } else {
            newValue = newValue + SCROLL_CHUNK_SIZE;
            newValue = newValue - (newValue % SCROLL_CHUNK_SIZE);
        }
    }

    if (controllerGetButtonDown(0, L_JPAD)) {
        newValue = newValue - 1;
        newValue = newValue - (newValue % SCROLL_CHUNK_SIZE);
    }

    if (newValue < 0) {
        newValue = 0;
    }

    if (newValue > 0xFFFF) {
        newValue = 0xFFFF;
    }

    *settingValue = newValue;
    *sliderValue = (float)newValue / 0xFFFF;
}

enum MenuDirection joystickOptionsUpdate(struct JoystickOptions* joystickOptions) {
    int controllerDir = controllerGetDirectionDown(0);

    if (controllerGetButtonDown(0, B_BUTTON)) {
        return MenuDirectionUp;
    }

    if (controllerDir & ControllerDirectionDown) {
        ++joystickOptions->selectedItem;

        if (joystickOptions->selectedItem == JoystickOptionCount) {
            joystickOptions->selectedItem = 0;
        }
    }

    if (controllerDir & ControllerDirectionUp) {
        if (joystickOptions->selectedItem == 0) {
            joystickOptions->selectedItem = JoystickOptionCount - 1;
        } else {
            --joystickOptions->selectedItem;
        }
    }

    switch (joystickOptions->selectedItem) {
        case JoystickOptionInvert:
            if (controllerGetButtonDown(0, A_BUTTON)) {
                joystickOptions->invertControls.checked = !joystickOptions->invertControls.checked;
                soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL);

                if (joystickOptions->invertControls.checked) {
                    gSaveData.controls.flags |= ControlSaveFlagsInvert;
                } else {
                    gSaveData.controls.flags &= ~ControlSaveFlagsInvert;
                }
            }

            break;
        case JoystickOptionSensitivity:
            joystickOptionsHandleSlider(&gSaveData.controls.sensitivity, &joystickOptions->lookSensitivity.value);
            return MenuDirectionStay;
        case JoystickOptionAcceleration:
            joystickOptionsHandleSlider(&gSaveData.controls.acceleration, &joystickOptions->lookAcceleration.value);
            return MenuDirectionStay;
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
    renderState->dl = menuCheckboxRender(&joystickOptions->invertControls, renderState->dl);
    gSPDisplayList(renderState->dl++, joystickOptions->lookSensitivity.back);
    renderState->dl = menuSliderRender(&joystickOptions->lookSensitivity, renderState->dl);

    gSPDisplayList(renderState->dl++, joystickOptions->lookAcceleration.back);
    renderState->dl = menuSliderRender(&joystickOptions->lookAcceleration, renderState->dl);

    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_ENV_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[DEJAVU_SANS_INDEX]);

    gDPPipeSync(renderState->dl++);
    menuSetRenderColor(renderState, joystickOptions->selectedItem == JoystickOptionInvert, &gSelectionGray, &gColorWhite);
    gSPDisplayList(renderState->dl++, joystickOptions->invertControls.text);

    gDPPipeSync(renderState->dl++);
    menuSetRenderColor(renderState, joystickOptions->selectedItem == JoystickOptionSensitivity, &gSelectionGray, &gColorWhite);
    gSPDisplayList(renderState->dl++, joystickOptions->lookSensitivityText);

    gDPPipeSync(renderState->dl++);
    menuSetRenderColor(renderState, joystickOptions->selectedItem == JoystickOptionAcceleration, &gSelectionGray, &gColorWhite);
    gSPDisplayList(renderState->dl++, joystickOptions->lookAccelerationText);

    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_INDEX]);
}