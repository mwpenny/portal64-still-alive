#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

#include <stdint.h>

#ifdef LIBDRAGON
    #include "libdragon/controller_buttons_libdragon.h"
#else
    #include "libultra/controller_buttons_libultra.h"
#endif

enum ControllerDirection {
    ControllerDirectionNone  = 0,
    ControllerDirectionUp    = (1 << 0),
    ControllerDirectionRight = (1 << 1),
    ControllerDirectionDown  = (1 << 2),
    ControllerDirectionLeft  = (1 << 3)
};

struct ControllerStick {
    int8_t x;
    int8_t y;
};

void controllersInit();
void controllersPoll();
int controllerIsConnected(int index);

void controllerSetRumble(int index, uint8_t enabled);

enum ControllerButtons controllerGetButtons(int index, enum ControllerButtons buttons);
enum ControllerButtons controllerGetButtonsDown(int index, enum ControllerButtons buttons);
enum ControllerButtons controllerGetButtonsUp(int index, enum ControllerButtons buttons);
enum ControllerButtons controllerGetButtonsHeld(int index, enum ControllerButtons buttons);

void controllerGetStick(int index, struct ControllerStick* stick);
enum ControllerDirection controllerGetDirection(int index);
enum ControllerDirection controllerGetDirectionDown(int index);

#endif
