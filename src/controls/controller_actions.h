#ifndef __CONTROLS_CONTROLLER_ACTIONS_H__
#define __CONTROLS_CONTROLLER_ACTIONS_H__

#include "math/vector2.h"

#include <stdint.h>

#define ACTION_IS_DIRECTION(action)         ((action) >= ControllerActionMove && (action) <= ControllerActionRotate)
#define MAX_BINDABLE_CONTROLLERS            2
#define MAX_SOURCES_PER_CONTROLLER_ACTION   4

enum ControllerActionInput {
    ControllerActionInputAButton,
    ControllerActionInputBButton,
    ControllerActionInputStartButton,

    ControllerActionInputCUpButton,
    ControllerActionInputCRightButton,
    ControllerActionInputCDownButton,
    ControllerActionInputCLeftButton,

    ControllerActionInputDUpButton,
    ControllerActionInputDRightButton,
    ControllerActionInputDDownButton,
    ControllerActionInputDLeftButton,

    ControllerActionInputZTrig,
    ControllerActionInputRTrig,
    ControllerActionInputLTrig,

    ControllerActionInputJoystick,

    ControllerActionInputCount
};

enum ControllerAction {
    ControllerActionNone,

    // Button actions
    ControllerActionOpenPortal0,
    ControllerActionOpenPortal1,
    ControllerActionJump,
    ControllerActionUseItem,
    ControllerActionDuck,
    ControllerActionLookForward,
    ControllerActionLookBackward,
    ControllerActionZoom,
    ControllerActionPause,

    // Direction actions
    ControllerActionMove,
    ControllerActionRotate,

    ControllerActionCount = ControllerActionRotate,
};

struct ControllerActionSource {
    uint8_t controllerIndex;
    enum ControllerActionInput input;
};

void controllerActionUpdate();

int controllerActionGet(enum ControllerAction action);
void controllerActionGetDirection(enum ControllerAction action, struct Vector2* direction);
void controllerActionMuteActive();

int controllerActionSources(enum ControllerAction action, struct ControllerActionSource* sources, int maxSources);
int controllerActionReadAnySource(struct ControllerActionSource* source);
int controllerActionSetSource(enum ControllerAction action, struct ControllerActionSource* source);
void controllerActionSetDefaultSources();

void controllerActionSetDeadzone(float percent);

#endif
