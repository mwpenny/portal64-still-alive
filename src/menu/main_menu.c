#include "main_menu.h"

#include "../util/memory.h"
#include "../util/rom.h"
#include "menu.h"
#include <string.h>

#include "../build/assets/materials/ui.h"
#include "../scene/render_plan.h"
#include "../controls/controller.h"

#include "../build/assets/test_chambers/test_chamber_00/test_chamber_00.h"
#include "../build/src/audio/clips.h"
#include "../build/src/audio/subtitles.h"

struct LandingMenuOption gMainMenuOptions[] = {
    {GAMEUI_NEWGAME, GameMenuStateNewGame},
    {GAMEUI_LOADGAME, GameMenuStateLoadGame},
    {GAMEUI_OPTIONS, GameMenuStateOptions},
};

void mainMenuReadCamera(struct GameMenu* gameMenu) {
    gScene.camera.transform = gScene.animator.armatures[TEST_CHAMBER_00_TEST_CHAMBER_00_ARMATURE_CAMERA].pose[0];
    vector3Scale(&gScene.camera.transform.position, &gScene.camera.transform.position, 1.0f / SCENE_SCALE);
    soundListenerUpdate(&gScene.camera.transform.position, &gScene.camera.transform.rotation, &gZeroVec, 0);
}

void mainMenuPlayAmbientSound() {
    static ALSndId soundId = -1;
    
    if (soundId == -1 || !soundPlayerIsPlaying(soundId)) {
        soundId = soundPlayerPlay(SOUNDS_PORTAL_PROCEDURAL_JIGGLE_BONE, 1.0f, 0.5f, NULL, NULL, SoundTypeMusic);
    }
}

void mainMenuInit(struct GameMenu* gameMenu) {
    sceneInitNoPauseMenu(&gScene, 1);

    gameMenuInit(gameMenu, gMainMenuOptions, sizeof(gMainMenuOptions) / sizeof(*gMainMenuOptions), 0);

    mainMenuReadCamera(gameMenu);

    gScene.camera.fov = 56.0f;

    mainMenuPlayAmbientSound();
}

void mainMenuUpdate(struct GameMenu* gameMenu) {
    if (!skAnimatorIsRunning(&gScene.animator.animators[TEST_CHAMBER_00_TEST_CHAMBER_00_ARMATURE_CAMERA])) {
        sceneAnimatorPlay(
            &gScene.animator, 
            TEST_CHAMBER_00_TEST_CHAMBER_00_ARMATURE_CAMERA, 
            TEST_CHAMBER_00_TEST_CHAMBER_00_CAMERA_ANIMATION__ANIM_CAMERA_MAIN_MENU_CAMERA, 
            1.0f,
            SKAnimatorFlagsLoop
        );
    }

    mainMenuReadCamera(gameMenu);
    sceneAnimatorUpdate(&gScene.animator);

    gameMenuUpdate(gameMenu);
    
    mainMenuPlayAmbientSound();
}

extern Lights1 gSceneLights;
extern LookAt gLookAt;

void mainMenuRender(struct GameMenu* gameMenu, struct RenderState* renderState, struct GraphicsTask* task) {
    gSPSetLights1(renderState->dl++, gSceneLights);
    LookAt* lookAt = renderStateRequestLookAt(renderState);

    if (!lookAt) {
        return;
    }
    
    *lookAt = gLookAt;
    gSPLookAt(renderState->dl++, lookAt);

    gDPSetRenderMode(renderState->dl++, G_RM_ZB_OPA_SURF, G_RM_ZB_OPA_SURF2);

    struct RenderPlan renderPlan;

    Mtx* staticMatrices = sceneAnimatorBuildTransforms(&gScene.animator, renderState);

    renderPlanBuild(&renderPlan, &gScene, renderState);
    renderPlanExecute(&renderPlan, &gScene, staticMatrices, renderState);

    gameMenuRender(gameMenu, renderState, task);
}
