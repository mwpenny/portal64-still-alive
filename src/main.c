#include "audio/soundplayer.h"
#include "controls/controller_actions.h"
#include "controls/rumble_pak_clip.h"
#include "graphics/graphics.h"
#include "graphics/profile_task.h"
#include "levels/credits.h"
#include "levels/intro.h"
#include "levels/levels.h"
#include "main.h"
#include "menu/main_menu.h"
#include "savefile/savefile.h"
#include "scene/dynamic_scene.h"
#include "scene/portal_surface.h"
#include "scene/scene.h"
#include "strings/translations.h"
#include "system/cartridge.h"
#include "system/controller.h"
#include "system/display.h"
#include "system/libultra/rsp_scheduler_libultra.h"
#include "util/dynamic_asset_loader.h"
#include "util/frame_time.h"
#include "util/memory.h"
#include "util/profile.h"

#ifdef PORTAL64_WITH_DEBUGGER
#include "debugger/debug.h"
#endif

#define MAX_FRAME_BUFFER_MESGS 8

OSMesgQueue         gfxFrameMsgQ;
static OSMesg       gfxFrameMsgBuf[MAX_FRAME_BUFFER_MESGS];
static OSScClient   gfxClient;

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

int main() {
    displayInit(1 /* interlaced */);

    osCreateMesgQueue(&gfxFrameMsgQ, gfxFrameMsgBuf, MAX_FRAME_BUFFER_MESGS);
    osScAddClient(rspSchedulerGet(), &gfxClient, &gfxFrameMsgQ);

    cartridgeInit();
    savefileLoad();

    u32 pendingGFX = 0;
    u32 drawBufferIndex = 0;
    u8 frameControl = 0;
    u8 inputIgnore = 5;
    u8 drawingEnabled = 0;

    u16* memoryEnd = graphicsInit((u16*)PHYS_TO_K0(osMemSize));
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
    frameTimeInit(displayGetFPS());
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
                    profileTask(&task->task.list, task->framebuffer);
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

    return 0;
}
