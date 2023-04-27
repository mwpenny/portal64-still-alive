#include "controller_actions.h"

#include "../util/memory.h"
#include "./controller.h"
#include "../math/mathf.h"

enum ControllerAction gDefaultControllerSettings[ControllerActionSourceCount] = {
    [ControllerActionSourceAButton] = ControllerActionJump,
    [ControllerActionSourceBButton] = ControllerActionUseItem,
    [ControllerActionSourceCUpButton] = ControllerActionMove,
    [ControllerActionSourceCRightButton] = ControllerActionMove,
    [ControllerActionSourceCDownButton] = ControllerActionMove,
    [ControllerActionSourceCLeftButton] = ControllerActionMove,
    [ControllerActionSourceDUpButton] = ControllerActionMove,
    [ControllerActionSourceDRightButton] = ControllerActionMove,
    [ControllerActionSourceDDownButton] = ControllerActionMove,
    [ControllerActionSourceDLeftButton] = ControllerActionMove,
    [ControllerActionSourceStartButton] = ControllerActionPause,
    [ControllerActionSourceLTrig] = ControllerActionOpenPortal1,
    [ControllerActionSourceRTrig] = ControllerActionOpenPortal1,
    [ControllerActionSourceZTrig] = ControllerActionOpenPortal0,
    [ControllerActionSourceJoystick] = ControllerActionRotate,
};

unsigned short gActionSourceButtonMask[ControllerActionSourceCount] = {
    [ControllerActionSourceAButton] = A_BUTTON,
    [ControllerActionSourceBButton] = B_BUTTON,
    [ControllerActionSourceCUpButton] = U_CBUTTONS,
    [ControllerActionSourceCRightButton] = R_CBUTTONS,
    [ControllerActionSourceCDownButton] = D_CBUTTONS,
    [ControllerActionSourceCLeftButton] = L_CBUTTONS,
    [ControllerActionSourceDUpButton] = U_JPAD,
    [ControllerActionSourceDRightButton] = R_JPAD,
    [ControllerActionSourceDDownButton] = D_JPAD,
    [ControllerActionSourceDLeftButton] = L_JPAD,
    [ControllerActionSourceStartButton] = START_BUTTON,
    [ControllerActionSourceLTrig] = L_TRIG,
    [ControllerActionSourceRTrig] = R_TRIG,
    [ControllerActionSourceZTrig] = Z_TRIG,
    [ControllerActionSourceJoystick] = 0,
};

enum ControllerAction gControllerSettings[2][ControllerActionSourceCount];

int gActionState = 0;
struct Vector2 gDirections[2];

void controllerActionInit() {
    memCopy(gControllerSettings[0], gDefaultControllerSettings, sizeof(gDefaultControllerSettings));
    zeroMemory(gControllerSettings[1], sizeof(gDefaultControllerSettings));
}

void controllerActionApply(enum ControllerAction action) {
    gActionState |= (1 << action);
}

#define DEADZONE_SIZE       5
#define MAX_JOYSTICK_RANGE  80

float controllerCleanupStickInput(s8 input) {
    if (input > -DEADZONE_SIZE && input < DEADZONE_SIZE) {
        return 0.0f;
    }

    if (input >= MAX_JOYSTICK_RANGE) {
        return 1.0f;
    }

    if (input <= -MAX_JOYSTICK_RANGE) {
        return -1.0f;
    }

    return ((float)input + (input > 0 ? -DEADZONE_SIZE : DEADZONE_SIZE)) * (1.0f / (MAX_JOYSTICK_RANGE - DEADZONE_SIZE));
}

void controllerActionReadDirection(enum ControllerActionSource source, int controllerIndex, int directionIndex) {
    struct Vector2 result = gDirections[directionIndex];

    switch (source) {
        case ControllerActionSourceCUpButton:
            if (controllerGetButton(controllerIndex, U_CBUTTONS)) {
                result.y += 1.0f;
            }
            if (controllerGetButton(controllerIndex, D_CBUTTONS)) {
                result.y -= 1.0f;
            }
            if (controllerGetButton(controllerIndex, R_CBUTTONS)) {
                result.x += 1.0f;
            }
            if (controllerGetButton(controllerIndex, L_CBUTTONS)) {
                result.x -= 1.0f;
            }
            break;
        case ControllerActionSourceDUpButton:
            if (controllerGetButton(controllerIndex, U_JPAD)) {
                result.y += 1.0f;
            }
            if (controllerGetButton(controllerIndex, D_JPAD)) {
                result.y -= 1.0f;
            }
            if (controllerGetButton(controllerIndex, R_JPAD)) {
                result.x += 1.0f;
            }
            if (controllerGetButton(controllerIndex, L_JPAD)) {
                result.x -= 1.0f;
            }
            break;
        case ControllerActionSourceJoystick:
        {
            OSContPad* pad = controllersGetControllerData(controllerIndex);
            result.x = controllerCleanupStickInput(pad->stick_x);
            result.y = controllerCleanupStickInput(pad->stick_y);
            break;
        }
        default:
            break;
    }

    gDirections[directionIndex] = result;
}

void controllerActionRead() {
    gActionState = 0;
    gDirections[0] = gZeroVec2;
    gDirections[1] = gZeroVec2;

    for (int controllerIndex = 0; controllerIndex < 2; ++controllerIndex) {
        for (int sourceIndex = 0; sourceIndex < ControllerActionSourceCount; ++sourceIndex) {
            enum ControllerAction action = gControllerSettings[controllerIndex][sourceIndex];

            if (IS_DIRECTION_ACTION(action)) {
                controllerActionReadDirection(sourceIndex, controllerIndex, action - ControllerActionMove);

                if (sourceIndex == ControllerActionSourceCUpButton || sourceIndex == ControllerActionSourceDUpButton) {
                    sourceIndex += 3;
                }
            } else if (controllerGetButtonDown(controllerIndex, gActionSourceButtonMask[sourceIndex])) {
                controllerActionApply(action);
            }
        }
    }

    for (int i = 0; i < 2; ++i) {
        gDirections[i].x = clampf(gDirections[i].x, -1.0f, 1.0f);
        gDirections[i].y = clampf(gDirections[i].y, -1.0f, 1.0f);
    }
}

struct Vector2 controllerDirectionGet(enum ControllerAction direction) {
    if (!IS_DIRECTION_ACTION(direction)) {
        return gZeroVec2;
    }

    return gDirections[direction - ControllerActionMove];
}

int controllerActionGet(enum ControllerAction action) {
    return (gActionState & (1 << action)) != 0;
}

int controllerSourcesForAction(enum ControllerAction action, struct ControllerSourceWithController* sources, int maxSources) {
    int index = 0;

    for (int controllerIndex = 0; controllerIndex < 2; ++controllerIndex) {
        for (int sourceIndex = 0; sourceIndex < ControllerActionSourceCount; ++sourceIndex) {
            if (gControllerSettings[controllerIndex][sourceIndex] == action) {
                sources[index].button = sourceIndex;
                sources[index].controller = controllerIndex;
                ++index;

                if (IS_DIRECTION_ACTION(action) && 
                    (sourceIndex == ControllerActionSourceCUpButton || sourceIndex == ControllerActionSourceDUpButton)) {
                    sourceIndex += 3;
                }

                if (index == maxSources) {
                    return index;
                }
            } 
        }
    }

    return index;
}