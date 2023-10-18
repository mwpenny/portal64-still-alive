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

    joystickOptions->invertControls = menuBuildCheckbox(&gDejaVuSansFont, "Invert Camera Pitch", JOYSTICK_X + 8, JOYSTICK_Y + 8);

    joystickOptions->invertControlsYaw = menuBuildCheckbox(&gDejaVuSansFont, "Invert Camera Yaw", JOYSTICK_X + 8, JOYSTICK_Y + 28);

    joystickOptions->tankControls = menuBuildCheckbox(&gDejaVuSansFont, "Tank Controls", JOYSTICK_X + 8, JOYSTICK_Y + 48);

    joystickOptions->lookSensitivityText = menuBuildText(&gDejaVuSansFont, "Look Sensitivity", JOYSTICK_X + 8, JOYSTICK_Y + 68);
    joystickOptions->lookSensitivity = menuBuildSlider(JOYSTICK_X + 120, JOYSTICK_Y + 68, 120, SCROLL_TICKS);

    joystickOptions->lookAccelerationText = menuBuildText(&gDejaVuSansFont, "Look Acceleration", JOYSTICK_X + 8, JOYSTICK_Y + 88);
    joystickOptions->lookAcceleration = menuBuildSlider(JOYSTICK_X + 120, JOYSTICK_Y + 88, 120, SCROLL_TICKS);

    joystickOptions->joystickDeadzoneText = menuBuildText(&gDejaVuSansFont, "Deadzone", JOYSTICK_X + 8, JOYSTICK_Y + 108);
    joystickOptions->joystickDeadzone = menuBuildSlider(JOYSTICK_X + 120, JOYSTICK_Y + 108, 120, SCROLL_TICKS);

    joystickOptions->invertControls.checked = (gSaveData.controls.flags & ControlSaveFlagsInvert) != 0;
    joystickOptions->invertControlsYaw.checked = (gSaveData.controls.flags & ControlSaveFlagsInvertYaw) != 0;
    joystickOptions->tankControls.checked = (gSaveData.controls.flags & ControlSaveTankControls) != 0;
    joystickOptions->lookSensitivity.value = (float)gSaveData.controls.sensitivity / 0xFFFF;
    joystickOptions->lookAcceleration.value = (float)gSaveData.controls.acceleration / 0xFFFF;
    joystickOptions->joystickDeadzone.value = (float)gSaveData.controls.deadzone / 0xFFFF;
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
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
    }

    if (controllerDir & ControllerDirectionUp) {
        if (joystickOptions->selectedItem == 0) {
            joystickOptions->selectedItem = JoystickOptionCount - 1;
        } else {
            --joystickOptions->selectedItem;
        }
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
    }

    switch (joystickOptions->selectedItem) {
        case JoystickOptionInvert:
            if (controllerGetButtonDown(0, A_BUTTON)) {
                joystickOptions->invertControls.checked = !joystickOptions->invertControls.checked;
                soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);

                if (joystickOptions->invertControls.checked) {
                    gSaveData.controls.flags |= ControlSaveFlagsInvert;
                } else {
                    gSaveData.controls.flags &= ~ControlSaveFlagsInvert;
                }
            }

            break;
        case JoystickOptionInvertYaw:
            if (controllerGetButtonDown(0, A_BUTTON)) {
                joystickOptions->invertControlsYaw.checked = !joystickOptions->invertControlsYaw.checked;
                soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);

                if (joystickOptions->invertControlsYaw.checked) {
                    gSaveData.controls.flags |= ControlSaveFlagsInvertYaw;
                } else {
                    gSaveData.controls.flags &= ~ControlSaveFlagsInvertYaw;
                }
            }

            break;
        case JoystickOptionTankControls:
            if (controllerGetButtonDown(0, A_BUTTON)) {
                joystickOptions->tankControls.checked = !joystickOptions->tankControls.checked;
                soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);

                if (joystickOptions->tankControls.checked) {
                    gSaveData.controls.flags |= ControlSaveTankControls;
                } else {
                    gSaveData.controls.flags &= ~ControlSaveTankControls;
                }
            }
            break;
        case JoystickOptionSensitivity:
            joystickOptionsHandleSlider(&gSaveData.controls.sensitivity, &joystickOptions->lookSensitivity.value);
            break;
        case JoystickOptionAcceleration:
            joystickOptionsHandleSlider(&gSaveData.controls.acceleration, &joystickOptions->lookAcceleration.value);
            break;
        case JoystickOptionDeadzone:
            joystickOptionsHandleSlider(&gSaveData.controls.deadzone, &joystickOptions->joystickDeadzone.value);
            controllerSetDeadzone(joystickOptions->joystickDeadzone.value * MAX_DEADZONE);
            break;
    }

    if (joystickOptions->selectedItem == JoystickOptionSensitivity ||
        joystickOptions->selectedItem == JoystickOptionAcceleration ||
        joystickOptions->selectedItem == JoystickOptionDeadzone){
        if ((controllerGetButtonDown(0, L_TRIG) || controllerGetButtonDown(0, Z_TRIG))) {
            return MenuDirectionLeft;
        }
        if ((controllerGetButtonDown(0, R_TRIG))) {
            return MenuDirectionRight;
        }
    }
    else{
        if (controllerDir & ControllerDirectionLeft || controllerGetButtonDown(0, L_TRIG) || controllerGetButtonDown(0, Z_TRIG)) {
            return MenuDirectionLeft;
        }
        if (controllerDir & ControllerDirectionRight || controllerGetButtonDown(0, R_TRIG)) {
            return MenuDirectionRight;
        }
    }

    return MenuDirectionStay;
}

void joystickOptionsRender(struct JoystickOptions* joystickOptions, struct RenderState* renderState, struct GraphicsTask* task) {
    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_ENV_INDEX]);
    
    gSPDisplayList(renderState->dl++, joystickOptions->invertControls.outline);
    renderState->dl = menuCheckboxRender(&joystickOptions->invertControls, renderState->dl);
    gSPDisplayList(renderState->dl++, joystickOptions->invertControlsYaw.outline);
    renderState->dl = menuCheckboxRender(&joystickOptions->invertControlsYaw, renderState->dl);
    gSPDisplayList(renderState->dl++, joystickOptions->tankControls.outline);
    renderState->dl = menuCheckboxRender(&joystickOptions->tankControls, renderState->dl);
    gSPDisplayList(renderState->dl++, joystickOptions->lookSensitivity.back);
    renderState->dl = menuSliderRender(&joystickOptions->lookSensitivity, renderState->dl);

    gSPDisplayList(renderState->dl++, joystickOptions->lookAcceleration.back);
    renderState->dl = menuSliderRender(&joystickOptions->lookAcceleration, renderState->dl);

    gSPDisplayList(renderState->dl++, joystickOptions->joystickDeadzone.back);
    renderState->dl = menuSliderRender(&joystickOptions->joystickDeadzone, renderState->dl);

    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_ENV_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[DEJAVU_SANS_INDEX]);

    gDPPipeSync(renderState->dl++);
    menuSetRenderColor(renderState, joystickOptions->selectedItem == JoystickOptionInvert, &gSelectionGray, &gColorWhite);
    gSPDisplayList(renderState->dl++, joystickOptions->invertControls.text);

    gDPPipeSync(renderState->dl++);
    menuSetRenderColor(renderState, joystickOptions->selectedItem == JoystickOptionInvertYaw, &gSelectionGray, &gColorWhite);
    gSPDisplayList(renderState->dl++, joystickOptions->invertControlsYaw.text);

    gDPPipeSync(renderState->dl++);
    menuSetRenderColor(renderState, joystickOptions->selectedItem == JoystickOptionTankControls, &gSelectionGray, &gColorWhite);
    gSPDisplayList(renderState->dl++, joystickOptions->tankControls.text);

    gDPPipeSync(renderState->dl++);
    menuSetRenderColor(renderState, joystickOptions->selectedItem == JoystickOptionSensitivity, &gSelectionGray, &gColorWhite);
    gSPDisplayList(renderState->dl++, joystickOptions->lookSensitivityText);

    gDPPipeSync(renderState->dl++);
    menuSetRenderColor(renderState, joystickOptions->selectedItem == JoystickOptionAcceleration, &gSelectionGray, &gColorWhite);
    gSPDisplayList(renderState->dl++, joystickOptions->lookAccelerationText);

    gDPPipeSync(renderState->dl++);
    menuSetRenderColor(renderState, joystickOptions->selectedItem == JoystickOptionDeadzone, &gSelectionGray, &gColorWhite);
    gSPDisplayList(renderState->dl++, joystickOptions->joystickDeadzoneText);

    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_INDEX]);
}
