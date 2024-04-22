#ifndef _CONTROLLER_H
#define _CONTROLLER_H

#include <stdint.h>

// The button values match those defined by libultra - see os_cont.h. Instead of
// hard-coding these values here, a better approach would be to have a dedicated
// file for each library implementation and choose the appropriate one at build
// time.
// NOTE: libdragon uses the same values but reversed - see joypad_buttons_t.
#define BUTTON_A       0x8000
#define BUTTON_B       0x4000
#define BUTTON_Z       0x2000
#define BUTTON_START   0x1000
#define BUTTON_UP      0x0800
#define BUTTON_DOWN    0x0400
#define BUTTON_LEFT    0x0200
#define BUTTON_RIGHT   0x0100
#define BUTTON_L       0x0020
#define BUTTON_R       0x0010
#define BUTTON_C_UP    0x0008
#define BUTTON_C_DOWN  0x0004
#define BUTTON_C_LEFT  0x0002
#define BUTTON_C_RIGHT 0x0001

void controllersInit();
void controllersSavePreviousState();
void controllersTriggerRead();

enum ControllerDirection {
    ControllerDirectionUp = (1 << 0),
    ControllerDirectionRight = (1 << 1),
    ControllerDirectionDown = (1 << 2),
    ControllerDirectionLeft = (1 << 3),
};

void controllersClearState();
int controllerIsConnected(int index);
int8_t   controllerGetStickX(int index);
int8_t   controllerGetStickY(int index);
uint16_t controllerGetLastButton(int index);
uint16_t controllerGetButton(int index, uint16_t button);
uint16_t controllerGetButtonDown(int index, uint16_t button);
uint16_t controllerGetButtonUp(int index, uint16_t button);
enum ControllerDirection controllerGetDirection(int index);
enum ControllerDirection controllerGetDirectionDown(int index);

void controllerHandlePlayback();

#endif