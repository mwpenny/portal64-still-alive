#include "system/controller.h"

#include "threads_libultra.h"
#include "util/memory.h"

#include <ultra64.h>

#define CONTROLLER_STACK_SIZE_BYTES 2048
#define STICK_DIRECTION_THRESHOLD   40

#define RUMBLE_MAX_PAK_COUNT        2
#define RUMBLE_RETRY_COUNT          3
#define RUMBLE_MAX_FAILURES         3

// For debugging
#define CONTROLLER_LOGGING_DISABLED 0
#define CONTROLLER_LOGGING_RECORD   1
#define CONTROLLER_LOGGING_PLAYBACK 2
#define CONTROLLER_LOGGING          CONTROLLER_LOGGING_DISABLED

enum RumblePakState {
    RumblePakStateUninitialized,
    RumblePakStateDisconnected,
    RumblePakStateInitialized
};

static OSThread                     sControllerThread;
static u64                          sControllerThreadStack[CONTROLLER_STACK_SIZE_BYTES / sizeof(u64)];

static OSMesgQueue                  sControllerDataQueue;
static OSMesg                       sControllerDataMesg;

static OSContStatus                 sControllerStatus[MAXCONTROLLERS];
static OSContPad                    sControllerData[MAXCONTROLLERS];
static u16                          sControllerLastButtons[MAXCONTROLLERS];
static enum ControllerDirection     sControllerLastDirection[MAXCONTROLLERS];

static OSPfs                        sRumblePakFs[RUMBLE_MAX_PAK_COUNT];
static enum RumblePakState          sRumblePakState[RUMBLE_MAX_PAK_COUNT];
static u8                           sCurrentRumbleValue[RUMBLE_MAX_PAK_COUNT];
static u8                           sTargetRumbleValue[RUMBLE_MAX_PAK_COUNT];
static u8                           sRumbleFailureCount[RUMBLE_MAX_PAK_COUNT];

#if CONTROLLER_LOGGING != CONTROLLER_LOGGING_DISABLED
#if CONTROLLER_LOGGING == CONTROLLER_LOGGING_RECORD
    #include "debugger/debug.h"
#elif CONTROLLER_LOGGING == CONTROLLER_LOGGING_PLAYBACK
    // Use tools/parse_controller_recording.py to generate this after recording
    #include "controller_recording.h"
#endif

void controllerUpdateLogging() {
#if CONTROLLER_LOGGING == CONTROLLER_LOGGING_RECORD
    uint8_t frame[offsetof(OSContPad, errno) * 2];
    memCopy(frame, &sControllerData[0], sizeof(frame) / 2);
    memCopy(frame + (sizeof(frame) / 2), &sControllerData[1], sizeof(frame) / 2);
    debug_dumpbinary(&frame, sizeof(frame));
#elif CONTROLLER_LOGGING == CONTROLLER_LOGGING_PLAYBACK
    static int sControllerDataPlaybackFrame = 0;
    if (sControllerDataPlaybackFrame < sizeof(gRecordedControllerData) / sizeof(*gRecordedControllerData)) {
        for (int i = 0; i < 2; ++i) {
            struct RecordedControllerData* frame = &gRecordedControllerData[sControllerDataPlaybackFrame++];
            sControllerData[i].button = frame->buttons;
            sControllerData[i].stick_x = frame->stick.x;
            sControllerData[i].stick_y = frame->stick.y;
            sControllerData[i].errno = 0;
        }
    }
#endif
}
#endif

static void controllerUpdateRumble(int index, OSContStatus* prevStatus, OSMesgQueue* serialMsgQ) {
    if (sControllerStatus[index].status != CONT_CARD_ON) {
        // Nothing in controller slot
        sRumblePakState[index] = RumblePakStateDisconnected;
        return;
    }

    if ((prevStatus[index].status != CONT_CARD_ON && sRumblePakState[index] == RumblePakStateDisconnected) ||
        sRumblePakState[index] == RumblePakStateUninitialized
    ) {
        // Device just inserted, or forcing re-initialization
        if (osMotorInit(serialMsgQ, &sRumblePakFs[index], index) != 0) {
            sRumblePakState[index] = RumblePakStateDisconnected;
            return;
        }

        sRumblePakState[index] = RumblePakStateInitialized;
        sRumbleFailureCount[index] = 0;
    }

    if (sTargetRumbleValue[index] != sCurrentRumbleValue[index]) {
        for (int i = 0; i < RUMBLE_RETRY_COUNT; ++i) {
            s32 rumbleError = sTargetRumbleValue[index] ? osMotorStart(&sRumblePakFs[index]) : osMotorStop(&sRumblePakFs[index]);

            if (rumbleError == PFS_ERR_CONTRFAIL) {
                if (i == (RUMBLE_RETRY_COUNT - 1)) {
                    ++sRumbleFailureCount[index];
                }
                if (sRumbleFailureCount[index] >= RUMBLE_MAX_FAILURES) {
                    sRumblePakState[index] = RumblePakStateUninitialized;
                }
            } else if (rumbleError != 0) {
                sRumblePakState[index] = RumblePakStateDisconnected;
                break;
            } else {
                sCurrentRumbleValue[index] = sTargetRumbleValue[index];
                sRumbleFailureCount[index] = 0;
                break;
            }
        }
    } else if (!sTargetRumbleValue[index]) {
        // Always send stop in case the message is lost while on
        osMotorStop(&sRumblePakFs[index]);
    }
}

static void controllerThreadLoop(void* arg) {
    OSMesgQueue serialMsgQ;
    OSMesg serialMsg;

    osCreateMesgQueue(&serialMsgQ, &serialMsg, 1);
    osSetEventMesg(OS_EVENT_SI, &serialMsgQ, NULL);

    OSContStatus prevStatus[MAXCONTROLLERS];
    OSContPad controllerData[MAXCONTROLLERS];

    while (1) {
        // Check the controller status
        memCopy(&prevStatus, &sControllerStatus, sizeof(sControllerStatus));
        osContStartQuery(&serialMsgQ);
        osRecvMesg(&serialMsgQ, NULL, OS_MESG_BLOCK);
        osContGetQuery(sControllerStatus);

        // Read the controller
        osContStartReadData(&serialMsgQ);
        osRecvMesg(&serialMsgQ, NULL, OS_MESG_BLOCK);
        osContGetReadData(controllerData);

        for (int i = 0; i < MAXCONTROLLERS; ++i) {
            if (sControllerStatus[i].errno != 0) {
                // Ignore controllers that aren't connected
                zeroMemory(&controllerData[i], sizeof(OSContPad));
            } else if (i < RUMBLE_MAX_PAK_COUNT) {
                controllerUpdateRumble(i, prevStatus, &serialMsgQ);
            }
        }

        // Add controller data to queue and block until it is recieved
        // I.e., wait until application calls controllersPoll()
        osSendMesg(&sControllerDataQueue, &controllerData, OS_MESG_BLOCK);
    }
}

void controllersInit() {
    OSMesgQueue serialMsgQ;
    OSMesg serialMsg;
    u8 validControllers;  // Ignored, we check plugged/unplugged dynamically

    osCreateMesgQueue(&serialMsgQ, &serialMsg, 1);
    osSetEventMesg(OS_EVENT_SI, &serialMsgQ, NULL);
    if (osContInit(&serialMsgQ, &validControllers, sControllerStatus) != 0) {
        return;
    }

    for (int i = 0; i < RUMBLE_MAX_PAK_COUNT; ++i) {
        sRumblePakState[i] = RumblePakStateUninitialized;
    }

    osCreateMesgQueue(&sControllerDataQueue, &sControllerDataMesg, 1);

    osCreateThread(
        &sControllerThread,
        CONTROLLER_THREAD_ID,
        controllerThreadLoop,
        0,
        sControllerThreadStack + (CONTROLLER_STACK_SIZE_BYTES / sizeof(u64)),
        (OSPri)CONTROLLER_PRIORITY
    );
    osStartThread(&sControllerThread);
}

void controllersPoll() {
    for (int i = 0; i < MAXCONTROLLERS; ++i) {
        sControllerLastButtons[i] = sControllerData[i].button;
        sControllerLastDirection[i] = controllerGetDirection(i);
    }

    // Just skip if controller data isn't ready yet
    OSMesg msg;
    if (osRecvMesg(&sControllerDataQueue, &msg, OS_MESG_NOBLOCK) == 0) {
        memCopy(&sControllerData, msg, sizeof(sControllerData));
    }

#if CONTROLLER_LOGGING != CONTROLLER_LOGGING_DISABLED
    controllerUpdateLogging();
#endif
}

int controllerIsConnected(int index) {
    return !sControllerStatus[index].errno;
}

void controllerSetRumble(int index, uint8_t enabled) {
    sTargetRumbleValue[index] = enabled;
}

enum ControllerButtons controllerGetButtons(int index, enum ControllerButtons buttons) {
    return sControllerData[index].button & buttons;
}

enum ControllerButtons controllerGetButtonsDown(int index, enum ControllerButtons buttons) {
    return sControllerData[index].button & ~sControllerLastButtons[index] & buttons;
}

enum ControllerButtons controllerGetButtonsUp(int index, enum ControllerButtons buttons) {
    return ~sControllerData[index].button & sControllerLastButtons[index] & buttons;
}

enum ControllerButtons controllerGetButtonsHeld(int index, enum ControllerButtons buttons) {
    return sControllerLastButtons[index] & buttons;
}

void controllerGetStick(int index, struct ControllerStick* stick) {
    stick->x = sControllerData[index].stick_x;
    stick->y = sControllerData[index].stick_y;
}

enum ControllerDirection controllerGetDirection(int index) {
    enum ControllerDirection result = ControllerDirectionNone;

    if (sControllerData[index].stick_y > STICK_DIRECTION_THRESHOLD || (sControllerData[index].button & ControllerButtonUp) != 0) {
        result |= ControllerDirectionUp;
    }

    if (sControllerData[index].stick_y < -STICK_DIRECTION_THRESHOLD || (sControllerData[index].button & ControllerButtonDown) != 0) {
        result |= ControllerDirectionDown;
    }

    if (sControllerData[index].stick_x > STICK_DIRECTION_THRESHOLD || (sControllerData[index].button & ControllerButtonRight) != 0) {
        result |= ControllerDirectionRight;
    }

    if (sControllerData[index].stick_x < -STICK_DIRECTION_THRESHOLD || (sControllerData[index].button & ControllerButtonLeft) != 0) {
        result |= ControllerDirectionLeft;
    }

    return result;
}

enum ControllerDirection controllerGetDirectionDown(int index) {
    return controllerGetDirection(index) & ~sControllerLastDirection[index];
}
