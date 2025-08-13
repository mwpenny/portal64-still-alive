#include "controller_actions.h"

#include "../util/memory.h"
#include "../system/controller.h"
#include "../math/mathf.h"
#include "../savefile/savefile.h"

unsigned char gDefaultControllerSettings[ControllerActionSourceCount] = {
    [ControllerActionSourceAButton] = ControllerActionOpenPortal0,
    [ControllerActionSourceBButton] = ControllerActionOpenPortal1,
    [ControllerActionSourceCUpButton] = ControllerActionMove,
    [ControllerActionSourceCRightButton] = ControllerActionNone,
    [ControllerActionSourceCDownButton] = ControllerActionNone,
    [ControllerActionSourceCLeftButton] = ControllerActionNone,
    [ControllerActionSourceDUpButton] = ControllerActionLookForward,
    [ControllerActionSourceDRightButton] = ControllerActionZoom,
    [ControllerActionSourceDDownButton] = ControllerActionLookBackward,
    [ControllerActionSourceDLeftButton] = ControllerActionNone,
    [ControllerActionSourceStartButton] = ControllerActionPause,
    [ControllerActionSourceLTrig] = ControllerActionDuck,
    [ControllerActionSourceRTrig] = ControllerActionUseItem,
    [ControllerActionSourceZTrig] = ControllerActionJump,
    [ControllerActionSourceJoystick] = ControllerActionRotate,
};

unsigned short gActionSourceButtonMask[ControllerActionSourceCount] = {
    [ControllerActionSourceAButton] = BUTTON_A,
    [ControllerActionSourceBButton] = BUTTON_B,
    [ControllerActionSourceCUpButton] = BUTTON_C_UP,
    [ControllerActionSourceCRightButton] = BUTTON_C_RIGHT,
    [ControllerActionSourceCDownButton] = BUTTON_C_DOWN,
    [ControllerActionSourceCLeftButton] = BUTTON_C_LEFT,
    [ControllerActionSourceDUpButton] = BUTTON_UP,
    [ControllerActionSourceDRightButton] = BUTTON_RIGHT,
    [ControllerActionSourceDDownButton] = BUTTON_DOWN,
    [ControllerActionSourceDLeftButton] = BUTTON_LEFT,
    [ControllerActionSourceStartButton] = BUTTON_START,
    [ControllerActionSourceLTrig] = BUTTON_L,
    [ControllerActionSourceRTrig] = BUTTON_R,
    [ControllerActionSourceZTrig] = BUTTON_Z,
    [ControllerActionSourceJoystick] = 0,
};

int gActionState = 0;
int gMutedActions = 0;
struct Vector2 gDirections[2];

#define ACTION_TO_BITMASK(action)       (1 << (action))

void controllerActionApply(enum ControllerAction action) {
    gActionState |= ACTION_TO_BITMASK(action);
}

#define DEADZONE_SIZE       5
#define MAX_JOYSTICK_RANGE  80

short gDeadzone = DEADZONE_SIZE;
float gDeadzoneScale = 1.0f / (MAX_JOYSTICK_RANGE - DEADZONE_SIZE);

void controllerSetDeadzone(float percent) {
    gDeadzone = (short)(percent * MAX_JOYSTICK_RANGE);
    gDeadzoneScale = 1.0f / (MAX_JOYSTICK_RANGE - gDeadzone);
}

float controllerCleanupStickInput(s8 input) {
    if (input > -gDeadzone && input < gDeadzone) {
        return 0.0f;
    }

    if (input >= MAX_JOYSTICK_RANGE) {
        return 1.0f;
    }

    if (input <= -MAX_JOYSTICK_RANGE) {
        return -1.0f;
    }

    return ((float)input + (input > 0 ? -gDeadzone : gDeadzone)) * gDeadzoneScale;
}

void controllerActionReadDirection(enum ControllerActionSource source, int controllerIndex, int directionIndex) {
    struct Vector2 result = gDirections[directionIndex];

    switch (source) {
        case ControllerActionSourceCUpButton:
            if (controllerGetButton(controllerIndex, BUTTON_C_UP)) {
                result.y += 1.0f;
            }
            if (controllerGetButton(controllerIndex, BUTTON_C_DOWN)) {
                result.y -= 1.0f;
            }
            if (controllerGetButton(controllerIndex, BUTTON_C_RIGHT)) {
                result.x += 1.0f;
            }
            if (controllerGetButton(controllerIndex, BUTTON_C_LEFT)) {
                result.x -= 1.0f;
            }
            break;
        case ControllerActionSourceDUpButton:
            if (controllerGetButton(controllerIndex, BUTTON_UP)) {
                result.y += 1.0f;
            }
            if (controllerGetButton(controllerIndex, BUTTON_DOWN)) {
                result.y -= 1.0f;
            }
            if (controllerGetButton(controllerIndex, BUTTON_RIGHT)) {
                result.x += 1.0f;
            }
            if (controllerGetButton(controllerIndex, BUTTON_LEFT)) {
                result.x -= 1.0f;
            }
            break;
        case ControllerActionSourceJoystick:
        {
            ControllerStick pad_stick = controllerGetStick(controllerIndex);
            result.x += controllerCleanupStickInput(pad_stick.x);
            result.y += controllerCleanupStickInput(pad_stick.y);
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

    int nextMutedState = 0;

    for (int controllerIndex = 0; controllerIndex < 2; ++controllerIndex) {
        for (int sourceIndex = 0; sourceIndex < ControllerActionSourceCount; ++sourceIndex) {
            enum ControllerAction action = gSaveData.controls.controllerSettings[controllerIndex][sourceIndex];

            if (IS_DIRECTION_ACTION(action)) {
                controllerActionReadDirection(sourceIndex, controllerIndex, action - ControllerActionMove);

                if (sourceIndex == ControllerActionSourceCUpButton || sourceIndex == ControllerActionSourceDUpButton) {
                    sourceIndex += 3;
                }
            } else if (IS_HOLDABLE_ACTION(action) && controllerGetButton(controllerIndex, gActionSourceButtonMask[sourceIndex])) {
                if (ACTION_TO_BITMASK(action) & gMutedActions) {
                    nextMutedState |= ACTION_TO_BITMASK(action);
                } else {
                    controllerActionApply(action);
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

    gMutedActions = nextMutedState;
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

void controllerActionMuteActive() {
    gMutedActions = gActionState;
}

int controllerSourcesForAction(enum ControllerAction action, struct ControllerSourceWithController* sources, int maxSources) {
    int index = 0;

    for (int controllerIndex = 0; controllerIndex < 2; ++controllerIndex) {
        for (int sourceIndex = 0; sourceIndex < ControllerActionSourceCount; ++sourceIndex) {
            if (gSaveData.controls.controllerSettings[controllerIndex][sourceIndex] == action) {
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

void controllerSetSource(enum ControllerAction action, enum ControllerActionSource source, int controller) {
    if (IS_DIRECTION_ACTION(action)) {
        source = controllerSourceMapToDirection(source);
        
    } else {
        source = controllerSourceMapAction(source);
    }

    if (source == ControllerActionSourceCount) {
        return;
    }

    if (source >= ControllerActionSourceCUpButton && 
        source <= ControllerActionSourceCLeftButton && 
        IS_DIRECTION_ACTION(gSaveData.controls.controllerSettings[controller][ControllerActionSourceCUpButton])) {
        gSaveData.controls.controllerSettings[controller][ControllerActionSourceCUpButton] = ControllerActionNone;
    }

    if (source >= ControllerActionSourceDUpButton && 
        source <= ControllerActionSourceDLeftButton && 
        IS_DIRECTION_ACTION(gSaveData.controls.controllerSettings[controller][ControllerActionSourceDUpButton])) {
        gSaveData.controls.controllerSettings[controller][ControllerActionSourceDUpButton] = ControllerActionNone;
    }

    gSaveData.controls.controllerSettings[controller][source] = action;

    if (IS_DIRECTION_ACTION(action) && (source == ControllerActionSourceCUpButton || source == ControllerActionSourceDUpButton)) {
        gSaveData.controls.controllerSettings[controller][source + 1] = ControllerActionNone;
        gSaveData.controls.controllerSettings[controller][source + 2] = ControllerActionNone;
        gSaveData.controls.controllerSettings[controller][source + 3] = ControllerActionNone;
    }
}

void controllerSetDefaultSource() {
    memCopy(gSaveData.controls.controllerSettings[0], gDefaultControllerSettings, sizeof(gDefaultControllerSettings));
    zeroMemory(gSaveData.controls.controllerSettings[1], sizeof(gDefaultControllerSettings));
}

struct ControllerSourceWithController controllerReadAnySource() {
    struct ControllerSourceWithController result;


    for (result.controller = 0; result.controller < 2; ++result.controller) {
        for (result.button = 0; result.button < ControllerActionSourceCount; ++result.button) {
            if (controllerGetButtonDown(result.controller, gActionSourceButtonMask[result.button])) {
                return result;
            }

            if (result.button == ControllerActionSourceJoystick) {
                ControllerStick pad_stick = controllerGetStick(result.controller);

                if (abs(pad_stick.x) > 40 || abs(pad_stick.y) > 40) {
                    return result;
                }
            }
        }
    }

    return result;
}

enum ControllerActionSource controllerSourceMapToDirection(enum ControllerActionSource source) {
    if (source >= ControllerActionSourceCUpButton && source <= ControllerActionSourceCLeftButton) {
        return ControllerActionSourceCUpButton;
    }

    if (source >= ControllerActionSourceDUpButton && source <= ControllerActionSourceDLeftButton) {
        return ControllerActionSourceDUpButton;
    }

    if (source == ControllerActionSourceJoystick) {
        return ControllerActionSourceJoystick;
    }

    return ControllerActionSourceCount;
}

enum ControllerActionSource controllerSourceMapAction(enum ControllerActionSource source) {
    if (source == ControllerActionSourceJoystick) {
        return ControllerActionSourceCount;
    }

    return source;
}