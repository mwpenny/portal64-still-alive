#include "controller_actions.h"

#include "math/mathf.h"
#include "savefile/savefile.h"
#include "system/controller.h"
#include "util/memory.h"
#include "util/sort.h"

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
            enum ControllerAction action = gSaveData.controls.controllerBindings[controllerIndex][inputIndex].action;

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
            enum ControllerAction action = gSaveData.controls.controllerBindings[controllerIndex][inputIndex].action;

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
    struct SortNode sorted[MAX_BINDABLE_CONTROLLERS * ControllerActionInputCount];

    int sourceCount = 0;
    for (int controllerIndex = 0; controllerIndex < MAX_BINDABLE_CONTROLLERS; ++controllerIndex) {
        for (int inputIndex = 0; inputIndex < ControllerActionInputCount; ++inputIndex) {
            struct SortedControllerAction* slot = &gSaveData.controls.controllerBindings[controllerIndex][inputIndex];

            if (slot->action != action) {
                continue;
            }

            sorted[sourceCount].index = (controllerIndex * ControllerActionInputCount) + inputIndex;
            sorted[sourceCount].sortOrder = slot->sortOrder;
            ++sourceCount;
        }
    }

    struct SortNode tmp[MAX_BINDABLE_CONTROLLERS * ControllerActionInputCount];
    mergeSort(sorted, tmp, 0, sourceCount);

    int sourceCountUnderLimit = MIN(maxSources, sourceCount);
    for (int i = 0; i < sourceCountUnderLimit; ++i) {
        sources[i].controllerIndex = (sorted[i].index / ControllerActionInputCount);
        sources[i].input = (sorted[i].index % ControllerActionInputCount);
    }

    return sourceCountUnderLimit;
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

static void setInputAction(int controllerIndex, enum ControllerActionInput input, enum ControllerAction action, int maxSources) {
    struct SortedControllerAction* dest = &gSaveData.controls.controllerBindings[controllerIndex][input];

    // Shift existing input sort orders
    for (controllerIndex = 0; controllerIndex < MAX_BINDABLE_CONTROLLERS; ++controllerIndex) {
        for (input = 0; input < ControllerActionInputCount; ++input) {
            struct SortedControllerAction* slot = &gSaveData.controls.controllerBindings[controllerIndex][input];

            if (slot->action == ControllerActionNone) {
                continue;
            }

            // Move inputs bound to the same action to make room at the front
            // If the new input isn't bound to this action yet, everything moves
            // If it is, only earlier inputs move to fill the gap
            if (slot->action == action && (dest->action != action || slot->sortOrder < dest->sortOrder)) {
                ++slot->sortOrder;

                if (slot->sortOrder >= maxSources) {
                    slot->action = ControllerActionNone;
                    slot->sortOrder = 0;
                }
            }

            // If binding the input unbinds it from a different action, move
            // that action's later inputs back to fill the gap
            if (slot->action == dest->action && dest->action != action && slot->sortOrder > dest->sortOrder) {
                --slot->sortOrder;
            }
        }
    }

    dest->action = action;
    dest->sortOrder = 0;
}

static int controllerActionSetDirectionSource(enum ControllerAction action, struct ControllerActionSource* source, int maxSources) {
    // If binding a button from a cluster (C buttons or D-pad), bind all of
    // its buttons. This is represented by binding the up button and leaving
    // the others unbound.
    if (INPUT_IS_C_BUTTON(source->input)) {
        setInputAction(source->controllerIndex, ControllerActionInputCUpButton,    action,               maxSources);
        setInputAction(source->controllerIndex, ControllerActionInputCRightButton, ControllerActionNone, maxSources);
        setInputAction(source->controllerIndex, ControllerActionInputCDownButton,  ControllerActionNone, maxSources);
        setInputAction(source->controllerIndex, ControllerActionInputCLeftButton,  ControllerActionNone, maxSources);
    } else if (INPUT_IS_D_PAD(source->input)) {
        setInputAction(source->controllerIndex, ControllerActionInputDUpButton,    action              , maxSources);
        setInputAction(source->controllerIndex, ControllerActionInputDRightButton, ControllerActionNone, maxSources);
        setInputAction(source->controllerIndex, ControllerActionInputDDownButton,  ControllerActionNone, maxSources);
        setInputAction(source->controllerIndex, ControllerActionInputDLeftButton,  ControllerActionNone, maxSources);
    } else if (source->input == ControllerActionInputJoystick) {
        setInputAction(source->controllerIndex, ControllerActionInputJoystick,     action,               maxSources);
    } else {
        return 0;
    }

    return 1;
}

static int controllerActionSetNonDirectionSource(enum ControllerAction action, struct ControllerActionSource* source, int maxSources) {
    if (source->input == ControllerActionInputJoystick) {
        return 0;
    }

    struct SortedControllerAction* bindings = gSaveData.controls.controllerBindings[source->controllerIndex];

    // When a button from a cluster (C buttons or D-pad) is bound to a direction
    // action, all of its buttons are bound. This is represented by binding
    // the up button and leaving the others unbound.
    //
    // Unbind button cluster from direction action if rebinding one of its
    // buttons (don't allow both types of actions at the same time)
    if (INPUT_IS_C_BUTTON(source->input) && ACTION_IS_DIRECTION(bindings[ControllerActionInputCUpButton].action)) {
        setInputAction(source->controllerIndex, ControllerActionInputCUpButton, ControllerActionNone, maxSources);
    } else if (INPUT_IS_D_PAD(source->input) && ACTION_IS_DIRECTION(bindings[ControllerActionInputDUpButton].action)) {
        setInputAction(source->controllerIndex, ControllerActionInputDUpButton, ControllerActionNone, maxSources);
    }

    setInputAction(source->controllerIndex, source->input, action, maxSources);
    return 1;
}

int controllerActionSetSource(enum ControllerAction action, struct ControllerActionSource* source, int maxSources) {
    int ret;

    if (ACTION_IS_DIRECTION(action)) {
        ret = controllerActionSetDirectionSource(action, source, maxSources);
    } else {
        ret = controllerActionSetNonDirectionSource(action, source, maxSources);
    }

    updateBoundControllers();
    return ret;
}

void controllerActionSetDefaultSources() {
    zeroMemory(gSaveData.controls.controllerBindings, sizeof(gSaveData.controls.controllerBindings));
    memCopy(gSaveData.controls.controllerBindings[0], sDefaultControllerBindings, sizeof(sDefaultControllerBindings));

    updateBoundControllers();
}

int controllerActionUsedControllerCount() {
    int count = 0;

    for (int i = 0; i < MAX_BINDABLE_CONTROLLERS; ++i) {
        if (sBoundControllers & INDEX_TO_BITMASK(i)) {
            ++count;
        }
    }

    return count;
}

int controllerActionUsesController(int controllerIndex) {
    return (sBoundControllers & INDEX_TO_BITMASK(controllerIndex)) != 0;
}

void controllerActionSetDeadzone(float percent) {
    sDeadzone = (short)(percent * MAX_DEADZONE_RANGE);
    sDeadzoneScale = 1.0f / (MAX_JOYSTICK_RANGE - sDeadzone);
}
