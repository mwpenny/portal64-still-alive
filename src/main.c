
#include <ultra64.h>
#include <sched.h>

#include "defs.h"
#include "graphics/graphics.h"
#include "util/rom.h"
#include "scene/scene.h"
#include "menu/main_menu.h"
#include "util/time.h"
#include "util/memory.h"
#include "string.h"
#include "controls/controller.h"
#include "controls/controller_actions.h"
#include "scene/dynamic_scene.h"
#include "audio/soundplayer.h"
#include "audio/audio.h"
#include "scene/portal_surface.h"
#include "sk64/skelatool_defs.h"
#include "levels/cutscene_runner.h"
#include "savefile/savefile.h"
#include "sk64/skelatool_animator.h"
#include "util/dynamic_asset_loader.h"

#include "levels/levels.h"
#include "savefile/checkpoint.h"

#ifdef PORTAL64_WITH_DEBUGGER
#include "../debugger/debugger.h"
#endif

static OSThread gameThread;
static OSThread initThread;

u64    mainStack[STACKSIZEBYTES/sizeof(u64)];
static u64 gameThreadStack[STACKSIZEBYTES/sizeof(u64)];
static u64 initThreadStack[STACKSIZEBYTES/sizeof(u64)];

static void gameProc(void *);
static void initProc(void *);

static OSMesg           PiMessages[DMA_QUEUE_SIZE];
static OSMesgQueue      PiMessageQ;

OSMesgQueue      gfxFrameMsgQ;
static OSMesg           gfxFrameMsgBuf[MAX_FRAME_BUFFER_MESGS];
static OSScClient       gfxClient;


OSSched scheduler;
u64            scheduleStack[OS_SC_STACKSIZE/8];
OSMesgQueue	*schedulerCommandQueue;

OSPiHandle	*gPiHandle;

void boot(void *arg) {
    osInitialize();

    gPiHandle = osCartRomInit();

    osCreateThread(
        &initThread, 
        1, 
        initProc, 
        NULL,
        (void *)(initThreadStack+(STACKSIZEBYTES/sizeof(u64))), 
		(OSPri)INIT_PRIORITY
    );

    osStartThread(&initThread);
}

static void initProc(void* arg) {
    osCreatePiManager(
        (OSPri) OS_PRIORITY_PIMGR, 
        &PiMessageQ,
        PiMessages,
        DMA_QUEUE_SIZE
    );

    osCreateThread(
        &gameThread, 
        6, 
        gameProc, 
        0, 
        gameThreadStack + (STACKSIZEBYTES/sizeof(u64)),
        (OSPri)GAME_PRIORITY
    );

    osStartThread(&gameThread);

    osSetThreadPri(NULL, 0);
    for(;;);
}

struct Scene gScene;
struct GameMenu gGameMenu;

extern OSMesgQueue dmaMessageQ;

extern char _heapStart[];

extern char _animation_segmentSegmentRomStart[];

typedef void (*InitCallback)(void* data);
typedef void (*UpdateCallback)(void* data);

struct SceneCallbacks {
    void* data;
    InitCallback initCallback;
    GraphicsCallback graphicsCallback;
    UpdateCallback updateCallback;
};

struct SceneCallbacks gTestChamberCallbacks = {
    .data = &gScene,
    .initCallback = (InitCallback)&sceneInit,
    .graphicsCallback = (GraphicsCallback)&sceneRender,
    .updateCallback = (UpdateCallback)&sceneUpdate,
};

struct SceneCallbacks gMainMenuCallbacks = {
    .data = &gGameMenu,
    .initCallback = (InitCallback)&mainMenuInit,
    .graphicsCallback = (GraphicsCallback)&mainMenuRender,
    .updateCallback = (UpdateCallback)&mainMenuUpdate,
};

struct SceneCallbacks* gSceneCallbacks = &gTestChamberCallbacks;

void levelLoadWithCallbacks(int levelIndex) {
    if (levelIndex == MAIN_MENU) {
        levelLoad(0);
        gSceneCallbacks = &gMainMenuCallbacks;
    } else {
        levelLoad(levelIndex);
        gSceneCallbacks = &gTestChamberCallbacks;
    }
}

static void gameProc(void* arg) {
    u8 schedulerMode = OS_VI_NTSC_LPF1;

    int fps = 60;

	switch (osTvType) {
		case 0: // PAL
			schedulerMode = HIGH_RES ? OS_VI_PAL_HPF1 : OS_VI_PAL_LPF1;
            fps = 50;
			break;
		case 1: // NTSC
			schedulerMode = HIGH_RES ? OS_VI_NTSC_HPF1 : OS_VI_NTSC_LPF1;
			break;
		case 2: // MPAL
            schedulerMode = HIGH_RES ? OS_VI_MPAL_HPF1 : OS_VI_MPAL_LPF1;
			break;
	}

    osCreateScheduler(
        &scheduler,
        (void *)(scheduleStack + OS_SC_STACKSIZE/8),
        SCHEDULER_PRIORITY,
        schedulerMode,
        1
    );

    schedulerCommandQueue = osScGetCmdQ(&scheduler);

    osCreateMesgQueue(&gfxFrameMsgQ, gfxFrameMsgBuf, MAX_FRAME_BUFFER_MESGS);
    osScAddClient(&scheduler, &gfxClient, &gfxFrameMsgQ);

	osViSetSpecialFeatures(OS_VI_GAMMA_OFF |
			OS_VI_GAMMA_DITHER_OFF |
			OS_VI_DIVOT_OFF |
			OS_VI_DITHER_FILTER_OFF);

    osViBlack(1);

    u32 pendingGFX = 0;
    u32 drawBufferIndex = 0;
    u8 frameControl = 0;
    u8 inputIgnore = 5;
    u8 drawingEnabled = 0;

    u16* memoryEnd = graphicsLayoutScreenBuffers((u16*)PHYS_TO_K0(osMemSize));

    gAudioHeapBuffer = (u8*)memoryEnd - AUDIO_HEAP_SIZE;

    memoryEnd = (u16*)gAudioHeapBuffer;

    heapInit(_heapStart, memoryEnd);
    romInit();

#ifdef PORTAL64_WITH_DEBUGGER
    OSThread* debugThreads[2];
    debugThreads[0] = &gameThread;
    gdbInitDebugger(gPiHandle, &dmaMessageQ, debugThreads, 1);
#endif

    dynamicSceneInit();
    contactSolverInit(&gContactSolver);
    portalSurfaceCleanupQueueInit();
    savefileLoad();
    levelLoadWithCallbacks(MAIN_MENU);
    gCurrentTestSubject = 0;
    cutsceneRunnerReset();
    controllersInit();
    initAudio(fps);
    soundPlayerInit();
    skSetSegmentLocation(CHARACTER_ANIMATION_SEGMENT, (unsigned)_animation_segmentSegmentRomStart);
    gSceneCallbacks->initCallback(gSceneCallbacks->data);

    while (1) {
        OSScMsg *msg = NULL;
        osRecvMesg(&gfxFrameMsgQ, (OSMesg*)&msg, OS_MESG_BLOCK);
        
        switch (msg->type) {
            case (OS_SC_RETRACE_MSG):
                // control the framerate
                frameControl = (frameControl + 1) % (FRAME_SKIP + 1);
                if (frameControl != 0) {
                    break;
                }

                if (levelGetQueued() != NO_QUEUED_LEVEL) {
                    if (pendingGFX == 0) {
                        soundPlayerStopAll();
                        dynamicSceneInit();
                        contactSolverInit(&gContactSolver);
                        portalSurfaceRevert(1);
                        portalSurfaceRevert(0);
                        portalSurfaceCleanupQueueInit();
                        heapInit(_heapStart, memoryEnd);
                        levelLoadWithCallbacks(levelGetQueued());
                        cutsceneRunnerReset();
                        dynamicAssetsReset();
                        gSceneCallbacks->initCallback(gSceneCallbacks->data);
                    }

                    break;
                }

                if (pendingGFX < 2 && drawingEnabled) {
                    graphicsCreateTask(&gGraphicsTasks[drawBufferIndex], gSceneCallbacks->graphicsCallback, gSceneCallbacks->data);
                    drawBufferIndex = drawBufferIndex ^ 1;
                    ++pendingGFX;
                }

                controllersTriggerRead();
                controllerHandlePlayback();
                controllerActionRead();
                skAnimatorSync();
                
                if (inputIgnore) {
                    --inputIgnore;
                } else {
                    gSceneCallbacks->updateCallback(gSceneCallbacks->data);
                    drawingEnabled = 1;
                }
                timeUpdateDelta();
                soundPlayerUpdate();
                controllersSavePreviousState();

                break;

            case (OS_SC_DONE_MSG):
                --pendingGFX;
                portalSurfaceCheckCleanupQueue();

                if (gScene.checkpointState == SceneCheckpointStatePendingRender) {
                    gScene.checkpointState = SceneCheckpointStateReady;
                }
                break;
            case (OS_SC_PRE_NMI_MSG):
                pendingGFX += 2;
                break;
            case SIMPLE_CONTROLLER_MSG:
                controllersReadPendingData();
                break;
        }
    }
}