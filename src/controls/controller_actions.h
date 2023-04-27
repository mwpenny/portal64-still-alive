#ifndef __CONTROLS_CONTROLLER_ACTIONS_H__
#define __CONTROLS_CONTROLLER_ACTIONS_H__

#include "../math/vector2.h"

enum ControllerActionSource {
    // face buttons
    ControllerActionSourceAButton,
    ControllerActionSourceBButton,
    ControllerActionSourceStartButton,
    // c buttons
    ControllerActionSourceCUpButton,
    ControllerActionSourceCRightButton,
    ControllerActionSourceCDownButton,
    ControllerActionSourceCLeftButton,
    // d pad
    ControllerActionSourceDUpButton,
    ControllerActionSourceDRightButton,
    ControllerActionSourceDDownButton,
    ControllerActionSourceDLeftButton,
    // triggers
    ControllerActionSourceZTrig,
    ControllerActionSourceRTrig,
    ControllerActionSourceLTrig,
    // joystick
    ControllerActionSourceJoystick,

    ControllerActionSourceCount,
};

enum ControllerAction {
    ControllerActionOpenNone,
    ControllerActionOpenPortal0,
    ControllerActionOpenPortal1,
    ControllerActionJump,
    ControllerActionUseItem,
    ControllerActionDuck,
    ControllerActionLookForward,
    ControllerActionLookBackward,
    ControllerActionPause,
    // direction actions
    ControllerActionMove,
    ControllerActionRotate,

    ControllerActionCount = ControllerActionRotate,
};

#define IS_DIRECTION_ACTION(action)     ((action) >= ControllerActionMove && (action) <= ControllerActionRotate)

struct ControllerSourceWithController {
    char button;
    char controller;
};

void controllerActionInit();

void controllerActionRead();

struct Vector2 controllerDirectionGet(enum ControllerAction direction);
int controllerActionGet(enum ControllerAction action);

int controllerSourcesForAction(enum ControllerAction action, struct ControllerSourceWithController* sources, int maxSources);

#endif