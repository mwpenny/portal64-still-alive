#ifndef _CONTROLLER_H
#define _CONTROLLER_H

#include <ultra64.h>

void controllersInit(void);
void controllersUpdate(void);
void controllersTriggerRead(void);

enum ControllerDirection {
    ControllerDirectionUp = (1 << 0),
    ControllerDirectionRight = (1 << 1),
    ControllerDirectionDown = (1 << 2),
    ControllerDirectionLeft = (1 << 3),
};

void controllersListen();
void controllersClearState();
int controllerHasPendingMessage();
int controllerIsConnected(int index);
OSContPad* controllersGetControllerData(int index);
u16 controllerGetLastButton(int index);
u16 controllerGetButton(int index, u16 button);
u16 controllerGetButtonDown(int index, u16 button);
u16 controllerGetButtonUp(int index, u16 button);
enum ControllerDirection controllerGetDirection(int index);
enum ControllerDirection controllerGetDirectionDown(int index);

#endif