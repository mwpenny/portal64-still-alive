#include "system/controller.h"

// TODO

void controllersInit() {
}

void controllersPoll() {
}

int controllerIsConnected(int index) {
    return 0;
}

void controllerSetRumble(int index, uint8_t enabled) {
}

enum ControllerButtons controllerGetButtons(int index, enum ControllerButtons buttons) {
    return ControllerButtonNone;
}

enum ControllerButtons controllerGetButtonsDown(int index, enum ControllerButtons buttons) {
    return ControllerButtonNone;
}

enum ControllerButtons controllerGetButtonsUp(int index, enum ControllerButtons buttons) {
    return ControllerButtonNone;
}

enum ControllerButtons controllerGetButtonsHeld(int index, enum ControllerButtons buttons) {
    return ControllerButtonNone;
}

void controllerGetStick(int index, struct ControllerStick* stick) {
    stick->x = 0;
    stick->y = 0;
}

enum ControllerDirection controllerGetDirection(int index) {
    return ControllerDirectionNone;
}

enum ControllerDirection controllerGetDirectionDown(int index) {
    return ControllerDirectionNone;
}
