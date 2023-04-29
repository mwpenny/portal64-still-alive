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
    ControllerActionNone,
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
#define IS_VALID_SOURCE(source) ((source) >= 0 && (source) < ControllerActionSourceCount)

struct ControllerSourceWithController {
    unsigned char button;
    unsigned char controller;
};

void controllerActionRead();

struct Vector2 controllerDirectionGet(enum ControllerAction direction);
int controllerActionGet(enum ControllerAction action);

int controllerSourcesForAction(enum ControllerAction action, struct ControllerSourceWithController* sources, int maxSources);

void controllerSetSource(enum ControllerAction action, enum ControllerActionSource source, int controller);
void controllerSetDefaultSource();

struct ControllerSourceWithController controllerReadAnySource();

enum ControllerActionSource controllerSourceMapToDirection(enum ControllerActionSource source);
enum ControllerActionSource controllerSourceMapAction(enum ControllerActionSource source);

#endif