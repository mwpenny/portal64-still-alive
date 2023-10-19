
#include "controller.h"
#include "defs.h"
#include "util/memory.h"
#include <sched.h>
#include "rumble_pak.h"
#include "../util/profile.h"

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
static u8 gRumblePakState;
static u8 gRumblePakOn;

static OSPfs gRumbleBackFs;

static OSContStatus  gControllerStatus[MAX_PLAYERS];
static OSContPad     gControllerData[MAX_PLAYERS];
static u16           gControllerLastButton[MAX_PLAYERS];
static enum ControllerDirection gControllerLastDirection[MAX_PLAYERS];
static int gControllerDeadFrames;
static int gTargetRumbleState;

static OSMesgQueue gControllerDataQueue;
static OSMesg gControllerDataMesg;

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

u8 gRumbleFailCount;

void controllersSavePreviousState() {
    for (unsigned i = 0; i < MAX_PLAYERS; ++i) {
        gControllerLastDirection[i] = controllerGetDirection(i);
        gControllerLastButton[i] = gControllerData[i].button;
    }
}

#define CONTROLLER_READ_SKIP_NUMBER 10

void controllersTriggerRead() {
    gTargetRumbleState = rumblePakCalculateState();

    OSMesg msg;
    if (osRecvMesg(&gControllerDataQueue, &msg, OS_MESG_NOBLOCK) == 0) {
        memCopy(&gControllerData, msg, sizeof(gControllerData));
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

void controllerCheckRumble(int prevStatus, OSMesgQueue* serialMsgQ) {
    if ((prevStatus != CONT_CARD_ON && gControllerStatus[0].status == CONT_CARD_ON && gRumblePakState == RumblepakStateDisconnected) || gRumblePakState == RumplepakStateUninitialized) {
        if (osMotorInit(serialMsgQ, &gRumbleBackFs, 0) == 0) {
            gRumblePakState = RumblepakStateInitialized;
        } else {
            gRumblePakState = RumblepakStateDisconnected;
        }
    } if (gControllerStatus[0].status != CONT_CARD_ON) {
        gRumblePakState = RumblepakStateDisconnected;
    }

    if (gRumblePakState == RumblepakStateInitialized) {
        if (gTargetRumbleState != gRumblePakOn) {
            for (int i = 0; i < 3; ++i) {
                s32 rumbleError = gTargetRumbleState ? osMotorStart(&gRumbleBackFs) : osMotorStop(&gRumbleBackFs);

                if (rumbleError == PFS_ERR_CONTRFAIL) {
                    if (i == 2) {
                        ++gRumbleFailCount;
                    }
                } else if (rumbleError != 0) {
                    gRumblePakState = RumblepakStateDisconnected;
                    break;
                } else {
                    gRumblePakOn = gTargetRumbleState;
                    gRumbleFailCount = 0;
                    break;
                }
            }
        } else if (!gTargetRumbleState) {
            osMotorStop(&gRumbleBackFs);
        }

        if (gRumbleFailCount >= 3) {
            gRumbleFailCount = 0;
            
            if (osMotorInit(serialMsgQ, &gRumbleBackFs, 0) == 0) {
                gRumblePakState = RumblepakStateInitialized;
            } else {
                gRumblePakState = RumplepakStateUninitialized;
            }
        }
    }
}

static void controllerThreadLoop(void* arg) {
    OSMesgQueue         serialMsgQ;
    OSMesg              serialMsg;

    osCreateMesgQueue(&serialMsgQ, &serialMsg, 1);
    osSetEventMesg(OS_EVENT_SI, &serialMsgQ, (OSMesg)1);

    OSContPad     controllerData[MAX_PLAYERS];

    for (;;) {
        OSMesg msgRead;
        // read the controller
        osContStartReadData(&serialMsgQ);
        osRecvMesg(&serialMsgQ, &msgRead, OS_MESG_BLOCK);
        osContGetReadData(&controllerData[0]);

        if (gControllerDeadFrames) {
            --gControllerDeadFrames;
            zeroMemory(controllerData, sizeof(controllerData));
        }

        // ingore controllers that aren't connected
        for (unsigned i = 0; i < MAX_PLAYERS; ++i) {
            if (gControllerStatus[i].errno & CONT_NO_RESPONSE_ERROR) {
                zeroMemory(&controllerData[i], sizeof(OSContPad));
            }
        }

        // add controller data to queue and block until it is recieved
        osSendMesg(&gControllerDataQueue, &controllerData, OS_MESG_BLOCK);

        int prevStatus = gControllerStatus[0].status;

        // check the controller status
        osContStartQuery(&serialMsgQ);
        osRecvMesg(&serialMsgQ, &msgRead, OS_MESG_BLOCK);
        osContGetQuery(&gControllerStatus[0]);

        controllerCheckRumble(prevStatus, &serialMsgQ);
    }
}

#define CONTROLLER_STACK_SIZE_BYTES 2048

static OSThread controllerThread;
static u64 controllerThreadStack[CONTROLLER_STACK_SIZE_BYTES/sizeof(u64)];

void controllersInit()
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

    osCreateMesgQueue(&gControllerDataQueue, &gControllerDataMesg, 1);

    osCreateThread(
        &controllerThread, 
        6, 
        controllerThreadLoop, 
        0, 
        controllerThreadStack + (CONTROLLER_STACK_SIZE_BYTES/sizeof(u64)),
        (OSPri)CONTROLLER_PRIORITY
    );

    osStartThread(&controllerThread);
}