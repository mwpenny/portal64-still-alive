#include <sched.h>
#include <ultra64.h>

#include "audio/soundplayer.h"
#include "controls/controller_actions.h"
#include "controls/rumble_pak_clip.h"
#include "graphics/graphics.h"
#include "graphics/profile_task.h"
#include "levels/cutscene_runner.h"
#include "levels/credits.h"
#include "levels/intro.h"
#include "levels/levels.h"
#include "main.h"
#include "menu/main_menu.h"
#include "savefile/checkpoint.h"
#include "savefile/savefile.h"
#include "scene/dynamic_scene.h"
#include "scene/portal_surface.h"
#include "scene/scene.h"
#include "sk64/skeletool_animator.h"
#include "strings/translations.h"
#include "system/cartridge.h"
#include "system/controller.h"
#include "system/defs.h"
#include "system/libultra/threads_libultra.h"
#include "system/screen.h"
#include "util/dynamic_asset_loader.h"
#include "util/frame_time.h"
#include "util/memory.h"
#include "util/profile.h"

#ifdef PORTAL64_WITH_DEBUGGER
#include "debugger/debug.h"
#endif

#define MAX_FRAME_BUFFER_MESGS 8

static OSThread idleThread;
static OSThread gameThread;

u64    mainStack[STACK_SIZE_BYTES/sizeof(u64)];
static u64 idleThreadStack[STACK_SIZE_BYTES/sizeof(u64)];
static u64 gameThreadStack[STACK_SIZE_BYTES/sizeof(u64)];

static void idleProc(void *);
static void gameProc(void *);

OSMesgQueue      gfxFrameMsgQ;
static OSMesg           gfxFrameMsgBuf[MAX_FRAME_BUFFER_MESGS];
static OSScClient       gfxClient;


OSSched scheduler;
u64            scheduleStack[OS_SC_STACKSIZE/8];
OSMesgQueue	*schedulerCommandQueue;
u8  schedulerMode;

void boot(void *arg) {
    osInitialize();

    osCreateThread(
        &idleThread,
        IDLE_THREAD_ID,
        idleProc,
        NULL,
        idleThreadStack + (STACK_SIZE_BYTES / sizeof(u64)),
        (OSPri)INIT_PRIORITY
    );

    osStartThread(&idleThread);
}

static void idleProc(void* arg) {
    cartridgeInit();

    osCreateThread(
        &gameThread, 
        GAME_THREAD_ID,
        gameProc, 
        0, 
        gameThreadStack + (STACK_SIZE_BYTES / sizeof(u64)),
        (OSPri)GAME_PRIORITY
    );

    osStartThread(&gameThread);

    osSetThreadPri(NULL, 0);
    for(;;);
}

struct Scene gScene;
struct GameMenu gGameMenu;
struct Intro gIntro;
struct Credits gCredits;

extern char _heapStart[];

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

struct SceneCallbacks gIntroCallbacks = {
    .data = &gIntro,
    .initCallback = (InitCallback)&introInit,
    .graphicsCallback = (GraphicsCallback)&introRender,
    .updateCallback = (UpdateCallback)&introUpdate,
};

struct SceneCallbacks gCreditsCallbacks = {
    .data = &gCredits,
    .initCallback = (InitCallback)&creditsInit,
    .graphicsCallback = (GraphicsCallback)&creditsRender,
    .updateCallback = (UpdateCallback)&creditsUpdate,
};

struct SceneCallbacks* gSceneCallbacks = &gTestChamberCallbacks;

void levelLoadWithCallbacks(int levelIndex) {
    if (levelIndex == CREDITS_MENU) {
        gSceneCallbacks = &gCreditsCallbacks;
    } else if (levelIndex == INTRO_MENU) {
        gSceneCallbacks = &gIntroCallbacks;
    } else if (levelIndex == MAIN_MENU) {
        levelLoad(0);
        gSceneCallbacks = &gMainMenuCallbacks;
    } else {
        levelLoad(levelIndex);
        gSceneCallbacks = &gTestChamberCallbacks;
    }

    levelClearQueued();
}

int updateSchedulerModeAndGetFPS(int interlacedMode) {
    int fps = 60;
    
    schedulerMode = interlacedMode ? OS_VI_NTSC_LPF1 : OS_VI_NTSC_LPN1;

    switch (osTvType) {
	case OS_TV_PAL:
		schedulerMode = HIGH_RES ? (interlacedMode ? OS_VI_PAL_HPF1 : OS_VI_PAL_HPN1) : (interlacedMode ? OS_VI_PAL_LPF1 : OS_VI_PAL_LPN1);
		fps = 50;
		break;
	case OS_TV_NTSC:
		schedulerMode = HIGH_RES ? (interlacedMode ? OS_VI_NTSC_HPF1 : OS_VI_NTSC_HPN1) : (interlacedMode ? OS_VI_NTSC_LPF1 : OS_VI_NTSC_LPN1);
		break;
	case OS_TV_MPAL:
		schedulerMode = HIGH_RES ? (interlacedMode ? OS_VI_MPAL_HPF1 : OS_VI_MPAL_HPN1) : (interlacedMode ? OS_VI_MPAL_LPF1 : OS_VI_MPAL_LPN1);
		break;
    }

    return fps;
}

int setViMode(int interlacedMode) {
    int fps = updateSchedulerModeAndGetFPS(interlacedMode);
    
    osViSetMode(&osViModeTable[schedulerMode]);
    
    osViSetSpecialFeatures(OS_VI_GAMMA_OFF |
		OS_VI_GAMMA_DITHER_OFF |
		OS_VI_DIVOT_OFF |
		OS_VI_DITHER_FILTER_OFF);

    return fps;
}

static void gameProc(void* arg) {
    int fps = updateSchedulerModeAndGetFPS(1);

    osCreateScheduler(
        &scheduler,
        (void *)(scheduleStack + OS_SC_STACKSIZE/8),
        SCHEDULER_PRIORITY,
        schedulerMode,
        1
    );

    osViSetSpecialFeatures(OS_VI_GAMMA_OFF |
		OS_VI_GAMMA_DITHER_OFF |
		OS_VI_DIVOT_OFF |
		OS_VI_DITHER_FILTER_OFF);

    schedulerCommandQueue = osScGetCmdQ(&scheduler);

    osCreateMesgQueue(&gfxFrameMsgQ, gfxFrameMsgBuf, MAX_FRAME_BUFFER_MESGS);
    osScAddClient(&scheduler, &gfxClient, &gfxFrameMsgQ);
    osViBlack(1);

    savefileLoad();

    u32 pendingGFX = 0;
    u32 drawBufferIndex = 0;
    u8 frameControl = 0;
    u8 inputIgnore = 5;
    u8 drawingEnabled = 0;

    u16* memoryEnd = graphicsLayoutScreenBuffers((u16*)PHYS_TO_K0(osMemSize));
    memoryEnd = soundPlayerInit(memoryEnd);
    heapInit(_heapStart, memoryEnd);

#ifdef PORTAL64_WITH_DEBUGGER
    debug_initialize();
#endif

    dynamicSceneInit();
    contactSolverInit(&gContactSolver);
    portalSurfaceCleanupQueueInit();
    
    levelLoadWithCallbacks(INTRO_MENU);
    controllersInit();
    controllerActionInit();
    rumblePakClipInit();
    frameTimeInit(fps);
    translationsLoad(gSaveData.video.textLanguage);
    gSceneCallbacks->initCallback(gSceneCallbacks->data);
    // this prevents the intro from crashing
    gGameMenu.currentRenderedLanguage = gSaveData.video.textLanguage;

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
                        profileClearAddressMap();
                        translationsLoad(gSaveData.video.textLanguage);
                        levelLoadWithCallbacks(levelGetQueued());
                        rumblePakClipInit();
                        dynamicAssetsReset();
                        menuResetDeferredQueue();
                        // if a portal fire button is being held
                        // don't fire portals until it is released
                        controllerActionMuteActive();
                        gSceneCallbacks->initCallback(gSceneCallbacks->data);
                    }

                    break;
                }

                if (translationsCurrentLanguage() != gGameMenu.currentRenderedLanguage) {
                    if (pendingGFX == 0) {
                        gameMenuRebuildText(&gGameMenu);
                    }

                    break;
                }

                Time startTime = timeGetTime();

                if (pendingGFX < 2 && drawingEnabled) {
                    Time renderStart = profileStart();
                    graphicsCreateTask(&gGraphicsTasks[drawBufferIndex], gSceneCallbacks->graphicsCallback, gSceneCallbacks->data);
                    profileEnd(renderStart, 1);
                    drawBufferIndex = drawBufferIndex ^ 1;
                    ++pendingGFX;
                }

                controllersPoll();
                rumblePakClipUpdate();
                controllerActionUpdate();
                romCopyAsyncDrain();
                
                if (inputIgnore) {
                    --inputIgnore;
                } else {
                    Time updateStart = profileStart();
                    gSceneCallbacks->updateCallback(gSceneCallbacks->data);
                    profileEnd(updateStart, 0);
                    drawingEnabled = 1;
                }
    
#if PORTAL64_WITH_RSP_PROFILER
                if (controllerGetButtonsDown(2, ControllerButtonDown)) {
                    struct GraphicsTask* task = &gGraphicsTasks[drawBufferIndex];
                    profileTask(&scheduler, &gameThread, &task->task.list, task->framebuffer);
                }
#endif

                soundPlayerUpdate();

                profileReport();

                gScene.cpuTime = timeGetTime() - startTime;

                break;

            case (OS_SC_DONE_MSG):
                --pendingGFX;
                portalSurfaceCheckCleanupQueue();
                menuTickDeferredQueue();

                if (gScene.checkpointState == SceneCheckpointStatePendingRender) {
                    gScene.checkpointState = SceneCheckpointStateReady;
                }

                frameTimeUpdate();
                break;
            case (OS_SC_PRE_NMI_MSG):
                pendingGFX += 2;
                break;
        }
    }
}