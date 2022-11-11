#ifndef _CONTROLLER_H
#define _CONTROLLER_H

#include <ultra64.h>

// 0 = disable 1 = record 2 = playbakc
#define CONTROLLER_LOG_CONTROLLER_DATA  2

void controllersInit(void);
void controllersReadPendingData(void);
void controllersSavePreviousState(void);
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

#if CONTROLLER_LOG_CONTROLLER_DATA
void controllerHandlePlayback();
#endif

#endif