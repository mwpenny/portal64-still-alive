#include "main_menu.h"

#include "../font/font.h"
#include "../font/dejavusans.h"
#include "../graphics/image.h"
#include "../util/memory.h"
#include "../util/rom.h"
#include "menu.h"
#include <string.h>

#include "../build/assets/materials/ui.h"
#include "../build/assets/materials/images.h"
#include "../scene/render_plan.h"
#include "../levels/levels.h"

#include "../build/assets/test_chambers/test_chamber_00/test_chamber_00.h"

#define PORTAL_LOGO_X   30
#define PORTAL_LOGO_Y   74

#define PORTAL_LOGO_WIDTH  128
#define PORTAL_LOGO_P_WIDTH  15
#define PORTAL_LOGO_O_WIDTH  30
#define PORTAL_LOGO_HEIGHT 47

struct Chapter gChapters[] = {
    {"CHAPTER 1", "Testchamber 00", images_chapter1_rgba_16b},
    {"CHAPTER 2", "Testchamber 04", images_chapter2_rgba_16b},
    {"CHAPTER 3", "Testchamber 08", images_chapter3_rgba_16b},
    {"CHAPTER 4", "Testchamber 10", images_chapter4_rgba_16b},
    {"CHAPTER 5", "Testchamber 13", images_chapter5_rgba_16b},
    {"CHAPTER 6", "Testchamber 14", images_chapter6_rgba_16b},
    {"CHAPTER 7", "Testchamber 15", images_chapter7_rgba_16b},
    {"CHAPTER 8", "Testchamber 16", images_chapter8_rgba_16b},
    {"CHAPTER 9", "Testchamber 17", images_chapter9_rgba_16b},
    {"CHAPTER 10", "Testchamber 18", images_chapter10_rgba_16b},
    {"CHAPTER 11", "Testchamber 19", images_chapter11_rgba_16b},
};

Gfx portal_logo_gfx[] = {
    gsSPTextureRectangle(
        PORTAL_LOGO_X << 2,
        PORTAL_LOGO_Y << 2,
        (PORTAL_LOGO_X + PORTAL_LOGO_P_WIDTH) << 2,
        (PORTAL_LOGO_Y + PORTAL_LOGO_HEIGHT) << 2,
        G_TX_RENDERTILE,
        0, 0,
        0x400, 0x400
    ),
    gsDPPipeSync(),
    gsDPSetEnvColor(60, 189, 236, 255),
    gsSPTextureRectangle(
        (PORTAL_LOGO_X + PORTAL_LOGO_P_WIDTH) << 2,
        PORTAL_LOGO_Y << 2,
        (PORTAL_LOGO_X + PORTAL_LOGO_P_WIDTH + PORTAL_LOGO_O_WIDTH) << 2,
        (PORTAL_LOGO_Y + PORTAL_LOGO_HEIGHT) << 2,
        G_TX_RENDERTILE,
        PORTAL_LOGO_P_WIDTH << 5, 0,
        0x400, 0x400
    ),
    gsDPPipeSync(),
    gsDPSetEnvColor(255, 255, 255, 255),
    gsSPTextureRectangle(
        (PORTAL_LOGO_X + PORTAL_LOGO_P_WIDTH + PORTAL_LOGO_O_WIDTH) << 2,
        PORTAL_LOGO_Y << 2,
        (PORTAL_LOGO_X + PORTAL_LOGO_WIDTH) << 2,
        (PORTAL_LOGO_Y + PORTAL_LOGO_HEIGHT) << 2,
        G_TX_RENDERTILE,
        (PORTAL_LOGO_P_WIDTH + PORTAL_LOGO_O_WIDTH) << 5, 0,
        0x400, 0x400
    ),
    gsSPEndDisplayList(),
};

void landingMenuInit(struct LandingMenu* landingMenu) {
    landingMenu->newGameText = menuBuildText(&gDejaVuSansFont, "NEW GAME", 30, 132);
    landingMenu->loadGameText = menuBuildText(&gDejaVuSansFont, "LOAD GAME", 30, 148);
    landingMenu->optionsText = menuBuildText(&gDejaVuSansFont, "OPTIONS", 30, 164);
}

void landingMenuRender(struct LandingMenu* landingMenu, struct RenderState* renderState, struct GraphicsTask* task) {
    gSPDisplayList(renderState->dl++, ui_material_list[DEFAULT_UI_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[PORTAL_LOGO_INDEX]);
    gSPDisplayList(renderState->dl++, portal_logo_gfx);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[PORTAL_LOGO_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[DEJAVU_SANS_INDEX]);
    gSPDisplayList(renderState->dl++, landingMenu->newGameText);
    gSPDisplayList(renderState->dl++, landingMenu->loadGameText);
    gSPDisplayList(renderState->dl++, landingMenu->optionsText);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_INDEX]);
}

#define CHAPTER_IMAGE_SIZE  (76 * 44 * 2)

void chapterMenuInit(struct ChapterMenu* chapterMenu, int x, int y) {
    chapterMenu->chapterText = malloc(sizeof(Gfx) * 3 * 10 + 1);
    chapterMenu->testChamberText = malloc(sizeof(Gfx) * 3 * 14 + 1);
    chapterMenu->border = menuBuildSolidBorder(
        x, y + 27, 
        92, 58,
        x + 5, y + 32,
        84, 48
    );

    chapterMenu->imageBuffer = malloc(CHAPTER_IMAGE_SIZE);

    zeroMemory(chapterMenu->imageBuffer, CHAPTER_IMAGE_SIZE);

    Gfx* dl = chapterMenu->chapterText;
    gSPEndDisplayList(dl++);

    dl = chapterMenu->testChamberText;
    gSPEndDisplayList(dl++);

    chapterMenu->chapter = NULL;

    chapterMenu->x = x;
    chapterMenu->y = y;
}

void chapterMenuSetChapter(struct ChapterMenu* chapterMenu, struct Chapter* chapter) {
    Gfx* dl = fontRender(
        &gDejaVuSansFont, 
        chapter->chapter, 
        chapterMenu->x, 
        chapterMenu->y, 
        chapterMenu->chapterText
    );
    gSPEndDisplayList(dl++);

    dl = fontRender(
        &gDejaVuSansFont, 
        chapter->testChamber, 
        chapterMenu->x, 
        chapterMenu->y + 14, 
        chapterMenu->testChamberText
    );
    gSPEndDisplayList(dl++);

    romCopy(chapter->imageData, chapterMenu->imageBuffer, CHAPTER_IMAGE_SIZE);
}

#define NEW_GAME_LEFT       40
#define NEW_GAME_TOP        45

void newGameInit(struct NewGameMenu* newGameMenu) {
    newGameMenu->menuOutline = menuBuildBorder(NEW_GAME_LEFT, NEW_GAME_TOP, SCREEN_WD - NEW_GAME_LEFT * 2, SCREEN_HT - NEW_GAME_TOP * 2);

    newGameMenu->newGameText = menuBuildText(&gDejaVuSansFont, "NEW GAME", 48, 48);
    
    newGameMenu->topLine = menuBuildHorizontalLine(52, 64, 214);
    newGameMenu->bottomLine = menuBuildHorizontalLine(52, 158, 220);

    newGameMenu->cancelButton = menuBuildButton(&gDejaVuSansFont, "Cancel", 222, 169, 46, 16);

    chapterMenuInit(&newGameMenu->chapter0, 55, 71);
    chapterMenuInit(&newGameMenu->chapter1, 163, 71);

    chapterMenuSetChapter(&newGameMenu->chapter0, &gChapters[0]);
    chapterMenuSetChapter(&newGameMenu->chapter1, &gChapters[1]);
}

void newGameRender(struct NewGameMenu* newGameMenu, struct RenderState* renderState, struct GraphicsTask* task) {
    gSPDisplayList(renderState->dl++, ui_material_list[DEFAULT_UI_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);
    gDPFillRectangle(renderState->dl++, 0, 0, SCREEN_WD, SCREEN_HT);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[ROUNDED_CORNERS_INDEX]);
    gSPDisplayList(renderState->dl++, newGameMenu->menuOutline);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[ROUNDED_CORNERS_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_ENV_INDEX]);
    gSPDisplayList(renderState->dl++, newGameMenu->topLine);
    gSPDisplayList(renderState->dl++, newGameMenu->bottomLine);
    gSPDisplayList(renderState->dl++, newGameMenu->cancelButton.outline);

    gDPPipeSync(renderState->dl++);
    gDPSetEnvColor(renderState->dl++, 0, 0, 0, 255);
    gSPDisplayList(renderState->dl++, newGameMenu->chapter0.border);

    gDPPipeSync(renderState->dl++);
    gDPSetEnvColor(renderState->dl++, 0, 0, 0, 255);
    gSPDisplayList(renderState->dl++, newGameMenu->chapter1.border);

    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_ENV_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[DEJAVU_SANS_INDEX]);
    gSPDisplayList(renderState->dl++, newGameMenu->newGameText);
    gSPDisplayList(renderState->dl++, newGameMenu->cancelButton.text);

    gDPPipeSync(renderState->dl++);
    gDPSetEnvColor(renderState->dl++, 255, 255, 255, 255);
    gSPDisplayList(renderState->dl++, newGameMenu->chapter0.chapterText);
    gSPDisplayList(renderState->dl++, newGameMenu->chapter0.testChamberText);

    gDPPipeSync(renderState->dl++);
    gDPSetEnvColor(renderState->dl++, 255, 255, 255, 255);
    gSPDisplayList(renderState->dl++, newGameMenu->chapter1.chapterText);
    gSPDisplayList(renderState->dl++, newGameMenu->chapter1.testChamberText);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_INDEX]);

    graphicsCopyImage(
        renderState, newGameMenu->chapter0.imageBuffer, 
        84, 48, 
        0, 0, 
        newGameMenu->chapter0.x + 5,
        newGameMenu->chapter0.y + 28,
        84, 48,
        gColorWhite
    );

    graphicsCopyImage(
        renderState, newGameMenu->chapter1.imageBuffer, 
        84, 48, 
        0, 0, 
        newGameMenu->chapter1.x + 5,
        newGameMenu->chapter1.y + 28,
        84, 48,
        gColorWhite
    );
}

void mainMenuReadCamera(struct MainMenu* mainMenu) {
    gScene.camera.transform = gScene.animator.armatures[TEST_CHAMBER_00_TEST_CHAMBER_00_ARMATURE_CAMERA].pose[0];
    vector3Scale(&gScene.camera.transform.position, &gScene.camera.transform.position, 1.0f / SCENE_SCALE);
    soundListenerUpdate(&gScene.camera.transform.position, &gScene.camera.transform.rotation, &gZeroVec, 0);
}

void mainMenuInit(struct MainMenu* mainMenu) {
    sceneInit(&gScene);

    landingMenuInit(&mainMenu->landingMenu);
    newGameInit(&mainMenu->newGameMenu);

    mainMenuReadCamera(mainMenu);

    gScene.camera.fov = 56.0f;
}

void mainMenuUpdate(struct MainMenu* mainMenu) {
    if (!skAnimatorIsRunning(&gScene.animator.animators[TEST_CHAMBER_00_TEST_CHAMBER_00_ARMATURE_CAMERA])) {
        sceneAnimatorPlay(
            &gScene.animator, 
            TEST_CHAMBER_00_TEST_CHAMBER_00_ARMATURE_CAMERA, 
            TEST_CHAMBER_00_TEST_CHAMBER_00_CAMERA_ANIMATION__ANIM_CAMERA_MAIN_MENU_CAMERA, 
            1.0f,
            SKAnimatorFlagsLoop
        );
    }

    mainMenuReadCamera(mainMenu);
    sceneAnimatorUpdate(&gScene.animator);
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

    // landingMenuRender(&mainMenu->landingMenu, renderState, task);
    newGameRender(&mainMenu->newGameMenu, renderState, task);
}