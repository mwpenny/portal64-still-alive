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
#include "../controls/controller.h"

#include "../build/assets/test_chambers/test_chamber_00/test_chamber_00.h"

#define PORTAL_LOGO_X   30
#define PORTAL_LOGO_Y   74

#define PORTAL_LOGO_WIDTH  128
#define PORTAL_LOGO_P_WIDTH  15
#define PORTAL_LOGO_O_WIDTH  30
#define PORTAL_LOGO_HEIGHT 47

struct Chapter gChapters[] = {
    {"CHAPTER 1", "Testchamber 00", images_chapter1_rgba_16b, 0},
    {"CHAPTER 2", "Testchamber 04", images_chapter2_rgba_16b, 2},
    {"CHAPTER 3", "Testchamber 08", images_chapter3_rgba_16b, 4},
    {"CHAPTER 4", "Testchamber 10", images_chapter4_rgba_16b, 6},
    {"CHAPTER 5", "Testchamber 13", images_chapter5_rgba_16b, -1},
    {"CHAPTER 6", "Testchamber 14", images_chapter6_rgba_16b, -1},
    {"CHAPTER 7", "Testchamber 15", images_chapter7_rgba_16b, -1},
    {"CHAPTER 8", "Testchamber 16", images_chapter8_rgba_16b, -1},
    {"CHAPTER 9", "Testchamber 17", images_chapter9_rgba_16b, -1},
    {"CHAPTER 10", "Testchamber 18", images_chapter10_rgba_16b, -1},
    {"CHAPTER 11", "Testchamber 19", images_chapter11_rgba_16b, -1},
    {NULL, NULL, NULL},
};

#define CHAPTER_COUNT   (sizeof(gChapters) / sizeof(*gChapters))

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

struct Coloru8 gSelectionOrange = {255, 156, 0, 255};
struct Coloru8 gSelectionGray = {201, 201, 201, 255};

void menuSetRenderColor(struct RenderState* renderState, int isSelected, struct Coloru8* selected, struct Coloru8* defaultColor) {
    if (isSelected) {
        gDPSetEnvColor(renderState->dl++, selected->r, selected->g, selected->b, selected->a);
    } else {
        gDPSetEnvColor(renderState->dl++, defaultColor->r, defaultColor->g, defaultColor->b, defaultColor->a);
    }
}

void landingMenuInit(struct LandingMenu* landingMenu) {
    landingMenu->newGameText = menuBuildText(&gDejaVuSansFont, "NEW GAME", 30, 132);
    landingMenu->loadGameText = menuBuildText(&gDejaVuSansFont, "LOAD GAME", 30, 148);
    landingMenu->optionsText = menuBuildText(&gDejaVuSansFont, "OPTIONS", 30, 164);

    landingMenu->selectedItem = 0;
}

enum MainMenuState landingMenuUpdate(struct LandingMenu* landingMenu) {
    if ((controllerGetDirectionDown(0) & ControllerDirectionUp) != 0 && landingMenu->selectedItem > 0) {
        --landingMenu->selectedItem;
    }

    if ((controllerGetDirectionDown(0) & ControllerDirectionDown) != 0 && landingMenu->selectedItem < 2) {
        ++landingMenu->selectedItem;
    }

    if (controllerGetButtonDown(0, A_BUTTON)) {
        switch (landingMenu->selectedItem)
        {
            case 0:
                return MainMenuStateNewGame;
                break;
        }
    }

    return MainMenuStateLanding;
}

void landingMenuRender(struct LandingMenu* landingMenu, struct RenderState* renderState, struct GraphicsTask* task) {
    gSPDisplayList(renderState->dl++, ui_material_list[DEFAULT_UI_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[PORTAL_LOGO_INDEX]);
    gSPDisplayList(renderState->dl++, portal_logo_gfx);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[PORTAL_LOGO_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[DEJAVU_SANS_INDEX]);
    gDPPipeSync(renderState->dl++);
    menuSetRenderColor(renderState, landingMenu->selectedItem == 0, &gSelectionGray, &gColorWhite);
    gSPDisplayList(renderState->dl++, landingMenu->newGameText);
    gDPPipeSync(renderState->dl++);
    menuSetRenderColor(renderState, landingMenu->selectedItem == 1, &gSelectionGray, &gColorWhite);
    gSPDisplayList(renderState->dl++, landingMenu->loadGameText);
    gDPPipeSync(renderState->dl++);
    menuSetRenderColor(renderState, landingMenu->selectedItem == 2, &gSelectionGray, &gColorWhite);
    gSPDisplayList(renderState->dl++, landingMenu->optionsText);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_INDEX]);
}

#define CHAPTER_IMAGE_SIZE  (84 * 48 * 2)

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
    Gfx* dl;
    
    if (chapter->chapter) {
        dl = fontRender(
            &gDejaVuSansFont, 
            chapter->chapter, 
            chapterMenu->x, 
            chapterMenu->y, 
            chapterMenu->chapterText
        );
        gSPEndDisplayList(dl++);
    }

    if (chapter->testChamber) {
        dl = fontRender(
            &gDejaVuSansFont, 
            chapter->testChamber, 
            chapterMenu->x, 
            chapterMenu->y + 14, 
            chapterMenu->testChamberText
        );
        gSPEndDisplayList(dl++);
    }

    if (chapter->imageData) {
        romCopy(chapter->imageData, chapterMenu->imageBuffer, CHAPTER_IMAGE_SIZE);
    }

    chapterMenu->chapter = chapter;
}

#define NEW_GAME_LEFT       40
#define NEW_GAME_TOP        45

void newGameInit(struct NewGameMenu* newGameMenu) {
    newGameMenu->menuOutline = menuBuildBorder(NEW_GAME_LEFT, NEW_GAME_TOP, SCREEN_WD - NEW_GAME_LEFT * 2, SCREEN_HT - NEW_GAME_TOP * 2);

    newGameMenu->newGameText = menuBuildText(&gDejaVuSansFont, "NEW GAME", 48, 48);
    
    newGameMenu->topLine = menuBuildHorizontalLine(52, 64, 214);
    newGameMenu->bottomLine = menuBuildHorizontalLine(52, 162, 214);

    newGameMenu->cancelButton = menuBuildButton(&gDejaVuSansFont, "Cancel", 218, 169, 46, 16);

    chapterMenuInit(&newGameMenu->chapter0, 55, 71);
    chapterMenuInit(&newGameMenu->chapter1, 163, 71);

    chapterMenuSetChapter(&newGameMenu->chapter0, &gChapters[0]);
    chapterMenuSetChapter(&newGameMenu->chapter1, &gChapters[1]);

    newGameMenu->chapterOffset = 0;
    newGameMenu->selectedChapter = 0;
}

enum MainMenuState newGameUpdate(struct NewGameMenu* newGameMenu) {
    if (controllerGetButtonDown(0, B_BUTTON)) {
        return MainMenuStateLanding;
    }

    if (controllerGetButtonDown(0, A_BUTTON) && gChapters[newGameMenu->selectedChapter + 1].testChamberNumber >= 0) {
        struct Transform identityTransform;
        transformInitIdentity(&identityTransform);
        identityTransform.position.y = 1.0f;

        levelQueueLoad(gChapters[newGameMenu->selectedChapter].testChamberNumber, &identityTransform, &gZeroVec);
    }

    if ((controllerGetDirectionDown(0) & ControllerDirectionRight) != 0 && 
        newGameMenu->selectedChapter + 1 < CHAPTER_COUNT &&
        gChapters[newGameMenu->selectedChapter + 1].imageData) {
        newGameMenu->selectedChapter = newGameMenu->selectedChapter + 1;
    }

    if ((controllerGetDirectionDown(0) & ControllerDirectionLeft) != 0 && newGameMenu->selectedChapter > 0) {
        newGameMenu->selectedChapter = newGameMenu->selectedChapter - 1;
    }

    int nextChapterOffset = newGameMenu->selectedChapter & ~1;

    if (nextChapterOffset != newGameMenu->chapterOffset) {
        newGameMenu->chapterOffset = nextChapterOffset;

        chapterMenuSetChapter(&newGameMenu->chapter0, &gChapters[newGameMenu->chapterOffset]);
        chapterMenuSetChapter(&newGameMenu->chapter1, &gChapters[newGameMenu->chapterOffset + 1]);
    }

    return MainMenuStateNewGame;
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
    menuSetRenderColor(renderState, newGameMenu->selectedChapter == newGameMenu->chapterOffset, &gSelectionOrange, &gColorBlack);
    gSPDisplayList(renderState->dl++, newGameMenu->chapter0.border);

    if (newGameMenu->chapter1.chapter->imageData) {
        gDPPipeSync(renderState->dl++);
        menuSetRenderColor(renderState, newGameMenu->selectedChapter != newGameMenu->chapterOffset, &gSelectionOrange, &gColorBlack);
        gSPDisplayList(renderState->dl++, newGameMenu->chapter1.border);
    }

    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_ENV_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[DEJAVU_SANS_INDEX]);
    gSPDisplayList(renderState->dl++, newGameMenu->newGameText);
    gSPDisplayList(renderState->dl++, newGameMenu->cancelButton.text);

    gDPPipeSync(renderState->dl++);
    menuSetRenderColor(renderState, newGameMenu->selectedChapter == newGameMenu->chapterOffset, &gSelectionOrange, &gColorWhite);
    gSPDisplayList(renderState->dl++, newGameMenu->chapter0.chapterText);
    gSPDisplayList(renderState->dl++, newGameMenu->chapter0.testChamberText);

    if (newGameMenu->chapter1.chapter->imageData) {
        gDPPipeSync(renderState->dl++);
        menuSetRenderColor(renderState, newGameMenu->selectedChapter != newGameMenu->chapterOffset, &gSelectionOrange, &gColorWhite);
        gSPDisplayList(renderState->dl++, newGameMenu->chapter1.chapterText);
        gSPDisplayList(renderState->dl++, newGameMenu->chapter1.testChamberText);
        gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_INDEX]);
    }

    graphicsCopyImage(
        renderState, newGameMenu->chapter0.imageBuffer, 
        84, 48, 
        0, 0, 
        newGameMenu->chapter0.x + 5,
        newGameMenu->chapter0.y + 32,
        84, 48,
        gColorWhite
    );

    if (newGameMenu->chapter1.chapter->imageData) {
        graphicsCopyImage(
            renderState, newGameMenu->chapter1.imageBuffer, 
            84, 48, 
            0, 0, 
            newGameMenu->chapter1.x + 5,
            newGameMenu->chapter1.y + 32,
            84, 48,
            gColorWhite
        );
    }
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

    mainMenu->state = MainMenuStateLanding;

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

    switch (mainMenu->state) {
        case MainMenuStateLanding:
            mainMenu->state = landingMenuUpdate(&mainMenu->landingMenu);
            break;
        case MainMenuStateNewGame:
            mainMenu->state = newGameUpdate(&mainMenu->newGameMenu);
            break;
    }
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

    switch (mainMenu->state) {
        case MainMenuStateLanding:
            landingMenuRender(&mainMenu->landingMenu, renderState, task);
            break;
        case MainMenuStateNewGame:
            newGameRender(&mainMenu->newGameMenu, renderState, task);
            break;
    }
}