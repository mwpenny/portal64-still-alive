#include "controller_actions.h"

#include "math/mathf.h"
#include "savefile/savefile.h"
#include "system/controller.h"
#include "util/memory.h"

unsigned char gDefaultControllerSettings[ControllerActionSourceCount] = {
    [ControllerActionSourceAButton]         = ControllerActionOpenPortal0,
    [ControllerActionSourceBButton]         = ControllerActionOpenPortal1,
    [ControllerActionSourceCUpButton]       = ControllerActionMove,
    [ControllerActionSourceCRightButton]    = ControllerActionNone,
    [ControllerActionSourceCDownButton]     = ControllerActionNone,
    [ControllerActionSourceCLeftButton]     = ControllerActionNone,
    [ControllerActionSourceDUpButton]       = ControllerActionLookForward,
    [ControllerActionSourceDRightButton]    = ControllerActionZoom,
    [ControllerActionSourceDDownButton]     = ControllerActionLookBackward,
    [ControllerActionSourceDLeftButton]     = ControllerActionNone,
    [ControllerActionSourceStartButton]     = ControllerActionPause,
    [ControllerActionSourceLTrig]           = ControllerActionDuck,
    [ControllerActionSourceRTrig]           = ControllerActionUseItem,
    [ControllerActionSourceZTrig]           = ControllerActionJump,
    [ControllerActionSourceJoystick]        = ControllerActionRotate,
};

unsigned short gActionSourceButtonMask[ControllerActionSourceCount] = {
    [ControllerActionSourceAButton]         = ControllerButtonA,
    [ControllerActionSourceBButton]         = ControllerButtonB,
    [ControllerActionSourceCUpButton]       = ControllerButtonCUp,
    [ControllerActionSourceCRightButton]    = ControllerButtonCRight,
    [ControllerActionSourceCDownButton]     = ControllerButtonCDown,
    [ControllerActionSourceCLeftButton]     = ControllerButtonCLeft,
    [ControllerActionSourceDUpButton]       = ControllerButtonUp,
    [ControllerActionSourceDRightButton]    = ControllerButtonRight,
    [ControllerActionSourceDDownButton]     = ControllerButtonDown,
    [ControllerActionSourceDLeftButton]     = ControllerButtonLeft,
    [ControllerActionSourceStartButton]     = ControllerButtonStart,
    [ControllerActionSourceLTrig]           = ControllerButtonL,
    [ControllerActionSourceRTrig]           = ControllerButtonR,
    [ControllerActionSourceZTrig]           = ControllerButtonZ,
    [ControllerActionSourceJoystick]        = ControllerButtonNone,
};

int gActionState = 0;
int gMutedActions = 0;
struct Vector2 gDirections[2];

#define ACTION_TO_BITMASK(action)       (1 << (action))

void controllerActionApply(enum ControllerAction action) {
    gActionState |= ACTION_TO_BITMASK(action);
}

#define DEFAULT_DEADZONE_SIZE   5
#define JOYSTICK_MOVE_THRESHOLD 40
#define MAX_JOYSTICK_RANGE      80

short gDeadzone = DEFAULT_DEADZONE_SIZE;
float gDeadzoneScale = 1.0f / (MAX_JOYSTICK_RANGE - DEFAULT_DEADZONE_SIZE);

void controllerSetDeadzone(float percent) {
    gDeadzone = (short)(percent * MAX_JOYSTICK_RANGE);
    gDeadzoneScale = 1.0f / (MAX_JOYSTICK_RANGE - gDeadzone);
}

float controllerCleanupStickInput(int8_t input) {
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
            if (controllerGetButtons(controllerIndex, ControllerButtonCUp)) {
                result.y += 1.0f;
            }
            if (controllerGetButtons(controllerIndex, ControllerButtonCDown)) {
                result.y -= 1.0f;
            }
            if (controllerGetButtons(controllerIndex, ControllerButtonCRight)) {
                result.x += 1.0f;
            }
            if (controllerGetButtons(controllerIndex, ControllerButtonCLeft)) {
                result.x -= 1.0f;
            }
            break;
        case ControllerActionSourceDUpButton:
            if (controllerGetButtons(controllerIndex, ControllerButtonUp)) {
                result.y += 1.0f;
            }
            if (controllerGetButtons(controllerIndex, ControllerButtonDown)) {
                result.y -= 1.0f;
            }
            if (controllerGetButtons(controllerIndex, ControllerButtonRight)) {
                result.x += 1.0f;
            }
            if (controllerGetButtons(controllerIndex, ControllerButtonLeft)) {
                result.x -= 1.0f;
            }
            break;
        case ControllerActionSourceJoystick:
        {
            struct ControllerStick padStick;
            controllerGetStick(controllerIndex, &padStick);
            result.x += controllerCleanupStickInput(padStick.x);
            result.y += controllerCleanupStickInput(padStick.y);
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

    for (int controllerIndex = 0; controllerIndex < MAX_BINDABLE_CONTROLLERS; ++controllerIndex) {
        for (int sourceIndex = 0; sourceIndex < ControllerActionSourceCount; ++sourceIndex) {
            enum ControllerAction action = gSaveData.controls.controllerSettings[controllerIndex][sourceIndex];

            if (IS_DIRECTION_ACTION(action)) {
                controllerActionReadDirection(sourceIndex, controllerIndex, action - ControllerActionMove);

                if (sourceIndex == ControllerActionSourceCUpButton || sourceIndex == ControllerActionSourceDUpButton) {
                    sourceIndex += 3;
                }
            } else if (IS_HOLDABLE_ACTION(action) && controllerGetButtons(controllerIndex, gActionSourceButtonMask[sourceIndex])) {
                if (ACTION_TO_BITMASK(action) & gMutedActions) {
                    nextMutedState |= ACTION_TO_BITMASK(action);
                } else {
                    controllerActionApply(action);
                }
            } else if (controllerGetButtonsDown(controllerIndex, gActionSourceButtonMask[sourceIndex])) {
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

    for (int controllerIndex = 0; controllerIndex < MAX_BINDABLE_CONTROLLERS; ++controllerIndex) {
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
    zeroMemory(gSaveData.controls.controllerSettings, sizeof(gSaveData.controls.controllerSettings));
    memCopy(gSaveData.controls.controllerSettings[0], gDefaultControllerSettings, sizeof(gDefaultControllerSettings));
}

struct ControllerSourceWithController controllerReadAnySource() {
    struct ControllerSourceWithController result;

    for (result.controller = 0; result.controller < MAX_BINDABLE_CONTROLLERS; ++result.controller) {
        for (result.button = 0; result.button < ControllerActionSourceCount; ++result.button) {
            if (controllerGetButtonsDown(result.controller, gActionSourceButtonMask[result.button])) {
                return result;
            }

            if (result.button == ControllerActionSourceJoystick) {
                struct ControllerStick padStick;
                controllerGetStick(result.controller, &padStick);

                if (abs(padStick.x) > JOYSTICK_MOVE_THRESHOLD || abs(padStick.y) > JOYSTICK_MOVE_THRESHOLD) {
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