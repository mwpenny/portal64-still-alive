
#include "controller.h"
#include "defs.h"
#include "util/memory.h"
#include <sched.h>

// 0 = disable 1 = record 2 = playbakc
#define CONTROLLER_LOG_CONTROLLER_DATA  0

#if CONTROLLER_LOG_CONTROLLER_DATA
    #include "../debugger/serial.h"
#endif

#define MAX_PLAYERS 4

static u8    validcontrollers = 0;
static u8    cntrlReadInProg  = 0;

static OSContStatus  gControllerStatus[MAX_PLAYERS];
static OSContPad     gControllerData[MAX_PLAYERS];
OSScMsg       gControllerMessage;
static u16           gControllerLastButton[MAX_PLAYERS];
static enum ControllerDirection gControllerLastDirection[MAX_PLAYERS];
static int gControllerDeadFrames;

extern OSMesgQueue gfxFrameMsgQ;

#define REMAP_PLAYER_INDEX(index)   (index)
// #define REMAP_PLAYER_INDEX(index)   0

void controllersClearState() {
    zeroMemory(gControllerData, sizeof(gControllerData));
    zeroMemory(gControllerLastButton, sizeof(gControllerLastButton));
    zeroMemory(gControllerLastDirection, sizeof(gControllerLastDirection));

    gControllerDeadFrames = 30;
}

int controllerIsConnected(int index) {
    return !gControllerStatus[index].errno;
}

void controllersListen() {
    /**** Set up message and queue, for read completion notification ****/
    gControllerMessage.type = SIMPLE_CONTROLLER_MSG;
    osSetEventMesg(OS_EVENT_SI, &gfxFrameMsgQ, (OSMesg)&gControllerMessage);
}

void controllersInit(void)
{
    OSMesgQueue         serialMsgQ;
    OSMesg              serialMsg;
    s32                 i;

    osCreateMesgQueue(&serialMsgQ, &serialMsg, 1);
    osSetEventMesg(OS_EVENT_SI, &serialMsgQ, (OSMesg)1);

    if((i = osContInit(&serialMsgQ, &validcontrollers, &gControllerStatus[0])) != 0)
        return;
    
    /**** Set up message and queue, for read completion notification ****/
    gControllerMessage.type = SIMPLE_CONTROLLER_MSG;
    osSetEventMesg(OS_EVENT_SI, &gfxFrameMsgQ, (OSMesg)&gControllerMessage);
}


void controllersReadPendingData(void) {
    osContGetReadData(gControllerData);
    cntrlReadInProg = 0;

    if (gControllerDeadFrames) {
        --gControllerDeadFrames;
        zeroMemory(gControllerData, sizeof(gControllerData));
    }

    for (unsigned i = 0; i < MAX_PLAYERS; ++i) {
        if (gControllerStatus[i].errno & CONT_NO_RESPONSE_ERROR) {
            zeroMemory(&gControllerData[i], sizeof(OSContPad));
        }
    }
}

void controllersSavePreviousState(void) {
    for (unsigned i = 0; i < MAX_PLAYERS; ++i) {
        gControllerLastDirection[i] = controllerGetDirection(i);
        gControllerLastButton[i] = gControllerData[i].button;
    }
}

int controllerHasPendingMessage() {
    return cntrlReadInProg;
}

#define CONTROLLER_READ_SKIP_NUMBER 10

void controllersTriggerRead(void) {
    if (validcontrollers && !cntrlReadInProg) {
        cntrlReadInProg = CONTROLLER_READ_SKIP_NUMBER;
        osContStartReadData(&gfxFrameMsgQ);
    } else if (cntrlReadInProg) {
        --cntrlReadInProg;
    }
}

OSContPad* controllersGetControllerData(int index) {
    return &gControllerData[REMAP_PLAYER_INDEX(index)];
}

u16 controllerGetLastButton(int index) {
    return gControllerLastButton[REMAP_PLAYER_INDEX(index)];
}

u16 controllerGetButton(int index, u16 button) {
    return gControllerData[REMAP_PLAYER_INDEX(index)].button & button;
}

u16 controllerGetButtonDown(int index, u16 button) {
    return gControllerData[REMAP_PLAYER_INDEX(index)].button & ~gControllerLastButton[REMAP_PLAYER_INDEX(index)] & button;
}

u16 controllerGetButtonUp(int index, u16 button) {
    return ~gControllerData[REMAP_PLAYER_INDEX(index)].button & gControllerLastButton[REMAP_PLAYER_INDEX(index)] & button;
}

enum ControllerDirection controllerGetDirection(int index) {
    enum ControllerDirection result = 0;

    if (gControllerData[REMAP_PLAYER_INDEX(index)].stick_y > 40 || (gControllerData[REMAP_PLAYER_INDEX(index)].button & U_JPAD) != 0) {
        result |= ControllerDirectionUp;
    }

    if (gControllerData[REMAP_PLAYER_INDEX(index)].stick_y < -40 || (gControllerData[REMAP_PLAYER_INDEX(index)].button & D_JPAD) != 0) {
        result |= ControllerDirectionDown;
    }

    if (gControllerData[REMAP_PLAYER_INDEX(index)].stick_x > 40 || (gControllerData[REMAP_PLAYER_INDEX(index)].button & R_JPAD) != 0) {
        result |= ControllerDirectionRight;
    }

    if (gControllerData[REMAP_PLAYER_INDEX(index)].stick_x < -40 || (gControllerData[REMAP_PLAYER_INDEX(index)].button & L_JPAD) != 0) {
        result |= ControllerDirectionLeft;
    }

    return result;
}

enum ControllerDirection controllerGetDirectionDown(int index) {
    return controllerGetDirection(REMAP_PLAYER_INDEX(index)) & ~gControllerLastDirection[REMAP_PLAYER_INDEX(index)];
}


struct ControllerData {
    OSContPad contPad;
};

int currentFrame = 0;

#if CONTROLLER_LOG_CONTROLLER_DATA == 2
struct ControllerData gRecordedControllerData[] = {
    #include "controller-data.h"
};
#endif

void controllerHandlePlayback() {
#if CONTROLLER_LOG_CONTROLLER_DATA == 1
    struct ControllerData data;
    data.contPad = gControllerData[0];
    gdbSendMessage(GDBDataTypeControllerData, (char*)&data, sizeof(struct ControllerData));
#elif CONTROLLER_LOG_CONTROLLER_DATA == 2
    if (currentFrame < sizeof(gRecordedControllerData) / sizeof(*gRecordedControllerData)) {
        gControllerData[0] = gRecordedControllerData[currentFrame].contPad;
        ++currentFrame;
    }
#endif
}