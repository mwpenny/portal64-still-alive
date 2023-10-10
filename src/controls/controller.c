
#include "controller.h"
#include "defs.h"
#include "util/memory.h"
#include <sched.h>

#include "../debugger/serial.h"
#include <string.h>

// 0 = disable, 1 = record, 2 = playback
#define CONTROLLER_LOG_CONTROLLER_DATA  0

#if CONTROLLER_LOG_CONTROLLER_DATA
    #include "../debugger/serial.h"
#endif

#define MAX_PLAYERS 4

enum ControllerEventType {
    ControllerEventTypeNone,
    ControllerEventTypeData,
    ControllerEventTypeStatus,
};

enum RumblepakState {
    RumblepakStateDisconnected,
    RumplepakStateUninitialized,
    RumblepakStateInitialized,
};

static u8 gValidControllers = 0;
static u8 gControllerReadInProgress  = 0;
static u8 gLastControllerQuery = ControllerEventTypeNone;
static u8 gRumblePakState;
static u8 gRumblePakOn;
static u8 gRumbleDelay;

static OSPfs gRumbleBackFs;

static OSContStatus  gControllerStatus[MAX_PLAYERS];
static OSContPad     gControllerData[MAX_PLAYERS];
OSScMsg       gControllerMessage;
static u16           gControllerLastButton[MAX_PLAYERS];
static enum ControllerDirection gControllerLastDirection[MAX_PLAYERS];
static int gControllerDeadFrames;

OSMesgQueue gControllerMsgQ;
OSMesg gControllerMsg;

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

void controllersInit(void)
{
    OSMesgQueue         serialMsgQ;
    OSMesg              serialMsg;
    s32                 i;

    osCreateMesgQueue(&serialMsgQ, &serialMsg, 1);
    osSetEventMesg(OS_EVENT_SI, &serialMsgQ, (OSMesg)1);

    if((i = osContInit(&serialMsgQ, &gValidControllers, &gControllerStatus[0])) != 0)
        return;

    if (gControllerStatus[0].status == CONT_CARD_ON) {
        gRumblePakState = RumplepakStateUninitialized;
        gRumblePakOn = 0;
    }
    
    /**** Set up message and queue, for read completion notification ****/
    gControllerMessage.type = SIMPLE_CONTROLLER_MSG;
    osCreateMesgQueue(&gControllerMsgQ, &gControllerMsg, 1);
    osSetEventMesg(OS_EVENT_SI, &gControllerMsgQ, (OSMesg)&gControllerMessage);
}

int controllerGetTargetRumbleStatus() {
    return 0;
}

void controllerHandleMessage() {
    if (gLastControllerQuery == ControllerEventTypeData) {
        osContGetReadData(gControllerData);
        gControllerReadInProgress = 0;

        if (gControllerDeadFrames) {
            --gControllerDeadFrames;
            zeroMemory(gControllerData, sizeof(gControllerData));
        }

        for (unsigned i = 0; i < MAX_PLAYERS; ++i) {
            if (gControllerStatus[i].errno & CONT_NO_RESPONSE_ERROR) {
                zeroMemory(&gControllerData[i], sizeof(OSContPad));
            }
        }

        if (gRumblePakState != RumblepakStateInitialized) {
            osContStartQuery(&gControllerMsgQ);
            gControllerReadInProgress = 1;
            gLastControllerQuery = ControllerEventTypeStatus;
        }

    } else if (gLastControllerQuery == ControllerEventTypeStatus) {
        int prevStatus = gControllerStatus[0].status;

        osContGetQuery(&gControllerStatus[0]);
        gLastControllerQuery = ControllerEventTypeNone;

        if ((prevStatus != CONT_CARD_ON && gControllerStatus[0].status == CONT_CARD_ON && gRumblePakState == RumblepakStateDisconnected) || gRumblePakState == RumplepakStateUninitialized) {
            if (osMotorInit(&gControllerMsgQ, &gRumbleBackFs, 0) == 0) {
                gRumblePakState = RumblepakStateInitialized;
                gRumbleDelay = 16;
            } else {
                gRumblePakState = RumblepakStateDisconnected;
                gRumblePakOn = 0;
            }
        } if (gControllerStatus[0].status != CONT_CARD_ON) {
            gRumblePakState = RumblepakStateDisconnected;
            gRumblePakOn = 0;
        }
    }
}

void controllersReadPendingData(void) {
    OSMesg msg;
    if (osRecvMesg(&gControllerMsgQ, &msg, OS_MESG_NOBLOCK) != -1) {
        controllerHandleMessage();
    }

    if (gRumblePakState == RumblepakStateInitialized) {
        int targetRumbleStatus = controllerGetTargetRumbleStatus();

        if (gRumbleDelay > 0) {
            --gRumbleDelay;
            return;
        }

        if (targetRumbleStatus != gRumblePakOn) {
            s32 rumbleError = targetRumbleStatus ? osMotorStart(&gRumbleBackFs) : osMotorStop(&gRumbleBackFs);

            if (rumbleError == PFS_ERR_CONTRFAIL) {
                gRumblePakState = RumplepakStateUninitialized;
            } else if (rumbleError != 0) {
                gRumblePakState = RumblepakStateDisconnected;
                gRumblePakOn = 0;
            } else {
                gRumblePakOn = targetRumbleStatus;
            }
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
    return gControllerReadInProgress;
}

#define CONTROLLER_READ_SKIP_NUMBER 10

void controllersTriggerRead(void) {
    if (gValidControllers && !gControllerReadInProgress) {
        gControllerReadInProgress = CONTROLLER_READ_SKIP_NUMBER;
        gLastControllerQuery = ControllerEventTypeData;
        osContStartReadData(&gControllerMsgQ);
    } else if (gControllerReadInProgress) {
        --gControllerReadInProgress;
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