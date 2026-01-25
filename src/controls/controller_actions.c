#include "controller_actions.h"

#include "math/mathf.h"
#include "savefile/savefile.h"
#include "system/controller.h"
#include "util/memory.h"

#define INDEX_TO_BITMASK(index)         (1 << (index))
#define ACTION_DIRECTION_INDEX(action)  ((action) - ControllerActionMove)

#define ACTION_IS_HOLDABLE(action)      ((action) >= ControllerActionOpenPortal0 && (action) <= ControllerActionOpenPortal1)
#define INPUT_IS_C_BUTTON(input)        ((input) >= ControllerActionInputCUpButton && (input) <= ControllerActionInputCLeftButton)
#define INPUT_IS_D_PAD(input)           ((input) >= ControllerActionInputDUpButton && (input) <= ControllerActionInputDLeftButton)

#define MAX_JOYSTICK_RANGE              80
#define MAX_DEADZONE_RANGE              20
#define DEFAULT_DEADZONE_SIZE           5
#define JOYSTICK_MOVE_THRESHOLD         40

static uint8_t sBoundControllers = 0;

static int sActiveActions = 0;
static int sMutedActions = 0;

struct Vector2 sDirections[2];  // 0 = move, 1 = rotate
static short sDeadzone = DEFAULT_DEADZONE_SIZE;
static float sDeadzoneScale = 1.0f / (MAX_JOYSTICK_RANGE - DEFAULT_DEADZONE_SIZE);

static uint8_t sDefaultControllerBindings[ControllerActionInputCount] = {
    [ControllerActionInputAButton]         = ControllerActionOpenPortal0,
    [ControllerActionInputBButton]         = ControllerActionOpenPortal1,
    [ControllerActionInputStartButton]     = ControllerActionPause,

    [ControllerActionInputCUpButton]       = ControllerActionMove,
    [ControllerActionInputCRightButton]    = ControllerActionNone,
    [ControllerActionInputCDownButton]     = ControllerActionNone,
    [ControllerActionInputCLeftButton]     = ControllerActionNone,

    [ControllerActionInputDUpButton]       = ControllerActionLookForward,
    [ControllerActionInputDRightButton]    = ControllerActionZoom,
    [ControllerActionInputDDownButton]     = ControllerActionLookBackward,
    [ControllerActionInputDLeftButton]     = ControllerActionNone,

    [ControllerActionInputZTrig]           = ControllerActionJump,
    [ControllerActionInputRTrig]           = ControllerActionUseItem,
    [ControllerActionInputLTrig]           = ControllerActionDuck,

    [ControllerActionInputJoystick]        = ControllerActionRotate,
};

static enum ControllerButtons sActionInputButtonMask[ControllerActionInputCount] = {
    [ControllerActionInputAButton]         = ControllerButtonA,
    [ControllerActionInputBButton]         = ControllerButtonB,
    [ControllerActionInputStartButton]     = ControllerButtonStart,

    [ControllerActionInputCUpButton]       = ControllerButtonCUp,
    [ControllerActionInputCRightButton]    = ControllerButtonCRight,
    [ControllerActionInputCDownButton]     = ControllerButtonCDown,
    [ControllerActionInputCLeftButton]     = ControllerButtonCLeft,

    [ControllerActionInputDUpButton]       = ControllerButtonUp,
    [ControllerActionInputDRightButton]    = ControllerButtonRight,
    [ControllerActionInputDDownButton]     = ControllerButtonDown,
    [ControllerActionInputDLeftButton]     = ControllerButtonLeft,

    [ControllerActionInputZTrig]           = ControllerButtonZ,
    [ControllerActionInputRTrig]           = ControllerButtonR,
    [ControllerActionInputLTrig]           = ControllerButtonL,

    [ControllerActionInputJoystick]        = ControllerButtonNone,
};

static void updateBoundControllers() {
    sBoundControllers = 0;

    for (int controllerIndex = 0; controllerIndex < MAX_BINDABLE_CONTROLLERS; ++controllerIndex) {
        for (int inputIndex = 0; inputIndex < ControllerActionInputCount; ++inputIndex) {
            enum ControllerAction action = gSaveData.controls.controllerBindings[controllerIndex][inputIndex];

            if (action != ControllerActionNone) {
                sBoundControllers |= INDEX_TO_BITMASK(controllerIndex);
                break;
            }
        }
    }
}

static float normalizeJoystickValue(int8_t input) {
    if (input > -sDeadzone && input < sDeadzone) {
        return 0.0f;
    }

    if (input >= MAX_JOYSTICK_RANGE) {
        return 1.0f;
    }

    if (input <= -MAX_JOYSTICK_RANGE) {
        return -1.0f;
    }

    // Percentage that stick has moved outside of deadzone
    return ((float)input + (input > 0 ? -sDeadzone : sDeadzone)) * sDeadzoneScale;
}

static void controllerActionReadDirection(struct Vector2* direction, int controllerIndex, enum ControllerActionInput input) {
    // Button clusters (C buttons or D-pad) either have all buttons bound to
    // a direction action or none at all. This is represented by binding the
    // up button and leaving the others unbound.
    switch (input) {
        case ControllerActionInputCUpButton:
            if (controllerGetButtons(controllerIndex, ControllerButtonCUp)) {
                direction->y += 1.0f;
            }
            if (controllerGetButtons(controllerIndex, ControllerButtonCDown)) {
                direction->y -= 1.0f;
            }
            if (controllerGetButtons(controllerIndex, ControllerButtonCRight)) {
                direction->x += 1.0f;
            }
            if (controllerGetButtons(controllerIndex, ControllerButtonCLeft)) {
                direction->x -= 1.0f;
            }
            break;
        case ControllerActionInputDUpButton:
            if (controllerGetButtons(controllerIndex, ControllerButtonUp)) {
                direction->y += 1.0f;
            }
            if (controllerGetButtons(controllerIndex, ControllerButtonDown)) {
                direction->y -= 1.0f;
            }
            if (controllerGetButtons(controllerIndex, ControllerButtonRight)) {
                direction->x += 1.0f;
            }
            if (controllerGetButtons(controllerIndex, ControllerButtonLeft)) {
                direction->x -= 1.0f;
            }
            break;
        case ControllerActionInputJoystick:
        {
            struct ControllerStick padStick;
            controllerGetStick(controllerIndex, &padStick);
            direction->x += normalizeJoystickValue(padStick.x);
            direction->y += normalizeJoystickValue(padStick.y);
            break;
        }
        default:
            break;
    }
}

static void controllerActionApply(enum ControllerAction action) {
    sActiveActions |= INDEX_TO_BITMASK(action);
}

void controllerActionInit() {
    updateBoundControllers();
}

void controllerActionUpdate() {
    sActiveActions = 0;
    sDirections[0] = gZeroVec2;
    sDirections[1] = gZeroVec2;

    int nextMutedActions = 0;

    for (int controllerIndex = 0; controllerIndex < MAX_BINDABLE_CONTROLLERS; ++controllerIndex) {
        for (int inputIndex = 0; inputIndex < ControllerActionInputCount; ++inputIndex) {
            enum ControllerAction action = gSaveData.controls.controllerBindings[controllerIndex][inputIndex];

            if (action == ControllerActionNone) {
                continue;
            }

            if (ACTION_IS_DIRECTION(action)) {
                controllerActionReadDirection(&sDirections[ACTION_DIRECTION_INDEX(action)], controllerIndex, inputIndex);
            } else if (ACTION_IS_HOLDABLE(action) && controllerGetButtons(controllerIndex, sActionInputButtonMask[inputIndex])) {
                if (INDEX_TO_BITMASK(action) & sMutedActions) {
                    nextMutedActions |= INDEX_TO_BITMASK(action);
                } else {
                    controllerActionApply(action);
                }
            } else if (controllerGetButtonsDown(controllerIndex, sActionInputButtonMask[inputIndex])) {
                controllerActionApply(action);
            }
        }
    }

    for (int i = 0; i < 2; ++i) {
        sDirections[i].x = clampf(sDirections[i].x, -1.0f, 1.0f);
        sDirections[i].y = clampf(sDirections[i].y, -1.0f, 1.0f);
    }

    sMutedActions = nextMutedActions;
}

int controllerActionGet(enum ControllerAction action) {
    return (sActiveActions & INDEX_TO_BITMASK(action)) != 0;
}

void controllerActionGetDirection(enum ControllerAction action, struct Vector2* direction) {
    if (ACTION_IS_DIRECTION(action)) {
        *direction = sDirections[ACTION_DIRECTION_INDEX(action)];
    } else {
        *direction = gZeroVec2;
    }
}

void controllerActionMuteActive() {
    sMutedActions = sActiveActions;
}

int controllerActionSources(enum ControllerAction action, struct ControllerActionSource* sources, int maxSources) {
    int sourceIndex = 0;

    for (int controllerIndex = 0; controllerIndex < MAX_BINDABLE_CONTROLLERS; ++controllerIndex) {
        for (int inputIndex = 0; inputIndex < ControllerActionInputCount; ++inputIndex) {
            if (gSaveData.controls.controllerBindings[controllerIndex][inputIndex] != action) {
                continue;
            }

            sources[sourceIndex].controllerIndex = controllerIndex;
            sources[sourceIndex].input = inputIndex;
            ++sourceIndex;

            if (sourceIndex == maxSources) {
                return sourceIndex;
            }
        }
    }

    return sourceIndex;
}

int controllerActionReadAnySource(struct ControllerActionSource* source) {
    for (source->controllerIndex = 0; source->controllerIndex < MAX_BINDABLE_CONTROLLERS; ++source->controllerIndex) {
        for (source->input = 0; source->input < ControllerActionInputCount; ++source->input) {
            if (source->input == ControllerActionInputJoystick) {
                struct ControllerStick padStick;
                controllerGetStick(source->controllerIndex, &padStick);

                if (abs(padStick.x) > JOYSTICK_MOVE_THRESHOLD || abs(padStick.y) > JOYSTICK_MOVE_THRESHOLD) {
                    return 1;
                }
            } else if (controllerGetButtonsDown(source->controllerIndex, sActionInputButtonMask[source->input])) {
                return 1;
            }
        }
    }

    return 0;
}

static int controllerActionSetDirectionSource(enum ControllerAction action, struct ControllerActionSource* source) {
    uint8_t* bindings = gSaveData.controls.controllerBindings[source->controllerIndex];

    // If binding a button from a cluster (C buttons or D-pad), bind all of
    // its buttons. This is represented by binding the up button and leaving
    // the others unbound.
    if (INPUT_IS_C_BUTTON(source->input)) {
        bindings[ControllerActionInputCUpButton]    = action;
        bindings[ControllerActionInputCRightButton] = ControllerActionNone;
        bindings[ControllerActionInputCDownButton]  = ControllerActionNone;
        bindings[ControllerActionInputCLeftButton]  = ControllerActionNone;
    } else if (INPUT_IS_D_PAD(source->input)) {
        bindings[ControllerActionInputDUpButton]    = action;
        bindings[ControllerActionInputDRightButton] = ControllerActionNone;
        bindings[ControllerActionInputDDownButton]  = ControllerActionNone;
        bindings[ControllerActionInputDLeftButton]  = ControllerActionNone;
    } else if (source->input == ControllerActionInputJoystick) {
        bindings[ControllerActionInputJoystick]     = action;
    } else {
        return 0;
    }

    return 1;
}

static int controllerActionSetNonDirectionSource(enum ControllerAction action, struct ControllerActionSource* source) {
    if (source->input == ControllerActionInputJoystick) {
        return 0;
    }

    uint8_t* bindings = gSaveData.controls.controllerBindings[source->controllerIndex];

    // When a button from a cluster (C buttons or D-pad) is bound to a direction
    // action, all of its buttons are bound. This is represented by binding
    // the up button and leaving the others unbound.
    //
    // Unbind button cluster from direction action if rebinding one of its
    // buttons (don't allow both types of actions at the same time)
    if (INPUT_IS_C_BUTTON(source->input) && ACTION_IS_DIRECTION(bindings[ControllerActionInputCUpButton])) {
        bindings[ControllerActionInputCUpButton] = ControllerActionNone;
    } else if (INPUT_IS_D_PAD(source->input) && ACTION_IS_DIRECTION(bindings[ControllerActionInputDUpButton])) {
        bindings[ControllerActionInputDUpButton] = ControllerActionNone;
    }

    bindings[source->input] = action;
    return 1;
}

int controllerActionSetSource(enum ControllerAction action, struct ControllerActionSource* source) {
    int ret;

    if (ACTION_IS_DIRECTION(action)) {
        ret = controllerActionSetDirectionSource(action, source);
    } else {
        ret = controllerActionSetNonDirectionSource(action, source);
    }

    updateBoundControllers();
    return ret;
}

void controllerActionSetDefaultSources() {
    zeroMemory(gSaveData.controls.controllerBindings, sizeof(gSaveData.controls.controllerBindings));
    memCopy(gSaveData.controls.controllerBindings[0], sDefaultControllerBindings, sizeof(sDefaultControllerBindings));

    updateBoundControllers();
}

int controllerActionUsesController(int controllerIndex) {
    return (sBoundControllers & INDEX_TO_BITMASK(controllerIndex)) != 0;
}

void controllerActionSetDeadzone(float percent) {
    sDeadzone = (short)(percent * MAX_DEADZONE_RANGE);
    sDeadzoneScale = 1.0f / (MAX_JOYSTICK_RANGE - sDeadzone);
}
