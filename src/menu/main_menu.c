#include "main_menu.h"

#include "../font/font.h"
#include "../font/dejavusans.h"
#include "../util/memory.h"
#include <string.h>

#include "../build/assets/materials/ui.h"
#include "../scene/render_plan.h"
#include "../levels/levels.h"

#include "../build/assets/test_chambers/test_chamber_00/test_chamber_00.h"

Gfx* menuBuildText(struct Font* font, char* message, int x, int y) {
    Gfx* result = malloc(sizeof(Gfx) * (fontCountGfx(font, message) + 1));
    Gfx* dl = result;

    dl = fontRender(font, message, x, y, dl);
    gSPEndDisplayList(dl++);

    return result;
}

void mainMenuInit(struct MainMenu* mainMenu) {
    sceneInit(&gScene);

    mainMenu->newGameText = menuBuildText(&gDejaVuSansFont, "NEW GAME", 30, 135);
    mainMenu->loadGameText = menuBuildText(&gDejaVuSansFont, "LOAD GAME", 30, 147);
    mainMenu->optionsText = menuBuildText(&gDejaVuSansFont, "OPTIONS", 30, 159);

    struct Transform* cameraTransfrom = &gCurrentLevel->locations[TEST_CHAMBER_00_TEST_CHAMBER_00_LOCATION_CAMERA].transform;

    struct Quaternion gRelativeRotation;
    quatAxisAngle(&gUp, -M_PI * 0.5f, &gRelativeRotation);
    gScene.camera.transform = *cameraTransfrom;
    gScene.camera.fov = 61.9275f * 3.0f / 4.0f;
    quatMultiply(&cameraTransfrom->rotation, &gRelativeRotation, &gScene.camera.transform.rotation);
    soundListenerUpdate(&gScene.camera.transform.position, &gScene.camera.transform.rotation, &gZeroVec, 0);
}

void mainMenuUpdate(struct MainMenu* mainMenu) {

}

extern Lights1 gSceneLights;
extern LookAt gLookAt;

void mainMenuRender(struct MainMenu* mainMenu, struct RenderState* renderState, struct GraphicsTask* task) {
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

    gSPDisplayList(renderState->dl++, ui_material_list[DEFAULT_UI_INDEX]);
    gSPDisplayList(renderState->dl++, ui_material_list[DEJAVU_SANS_INDEX]);

    gSPDisplayList(renderState->dl++, mainMenu->newGameText);
    gSPDisplayList(renderState->dl++, mainMenu->loadGameText);
    gSPDisplayList(renderState->dl++, mainMenu->optionsText);
}