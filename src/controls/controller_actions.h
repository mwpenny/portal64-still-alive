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
    ControllerActionSourceLTrig,
    ControllerActionSourceRTrig,
    ControllerActionSourceZTrig,
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
};

void controllerActionInit();

void controllerActionRead();

struct Vector2 controllerDirectionGet(enum ControllerAction direction);
int controllerActionGet(enum ControllerAction action);

#endif