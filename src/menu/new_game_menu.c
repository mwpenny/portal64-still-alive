#include "new_game_menu.h"

#include <string.h>

#include "./translations.h"

#include "../font/font.h"
#include "../font/dejavusans.h"

#include "../graphics/image.h"
#include "../util/memory.h"
#include "../util/rom.h"
#include "../audio/soundplayer.h"
#include "./text_manipulation.h"

#include "../build/assets/materials/ui.h"
#include "../build/assets/materials/images.h"
#include "../build/src/audio/clips.h"
#include "../build/src/audio/subtitles.h"

#include "../levels/levels.h"
#include "../savefile/savefile.h"
#include "../controls/controller.h"

struct Chapter gChapters[] = {
    {images_chapter1_rgba_16b, 0, 0},
    {images_chapter2_rgba_16b, 2, 4},
    {images_chapter3_rgba_16b, 4, 8},
    {images_chapter4_rgba_16b, 6, 10},
    {images_chapter5_rgba_16b, -1, 13},
    {images_chapter6_rgba_16b, -1, 14},
    {images_chapter7_rgba_16b, -1, 15},
    {images_chapter8_rgba_16b, -1, 16},
    {images_chapter9_rgba_16b, -1, 17},
    {images_chapter10_rgba_16b, -1, 18},
    {images_chapter11_rgba_16b, -1, 19},
    {NULL, -1, 0},
};

#define MAX_CHAPTER_COUNT   ((sizeof(gChapters) / sizeof(*gChapters)) - 1)

struct Chapter* chapterFindForChamber(int chamberIndex) {
    for (int i = 1; i < MAX_CHAPTER_COUNT; ++i) {
        if (gChapters[i].testChamberNumber > chamberIndex) {
            return &gChapters[i - 1];
        }
    }

    return &gChapters[MAX_CHAPTER_COUNT - 1];
}

void chapterMenuInit(struct ChapterMenu* chapterMenu, int x, int y) {
    chapterMenu->chapterText = NULL;
    chapterMenu->testChamberText = NULL;
    chapterMenu->border = menuBuildSolidBorder(
        x, y + 27, 
        92, 58,
        x + 5, y + 32,
        CHAPTER_IMAGE_WIDTH, CHAPTER_IMAGE_HEIGHT
    );

    chapterMenu->imageBuffer = malloc(CHAPTER_IMAGE_SIZE);

    zeroMemory(chapterMenu->imageBuffer, CHAPTER_IMAGE_SIZE);

    chapterMenu->chapter = NULL;

    chapterMenu->x = x;
    chapterMenu->y = y;
}

void chapterMenuSetChapter(struct ChapterMenu* chapterMenu, struct Chapter* chapter, int chapterIndex) {
    menuFreePrerenderedDeferred(chapterMenu->chapterText);
    menuFreePrerenderedDeferred(chapterMenu->testChamberText);

    char chapterText[64];
    sprintf(chapterText, "%s %d", translationsGet(GAMEUI_CHAPTER), chapterIndex + 1);
    chapterMenu->chapterText = menuBuildPrerenderedText(&gDejaVuSansFont, chapterText, chapterMenu->x, chapterMenu->y, SCREEN_WD);

    textManipTestChamberMessage(chapterText, gChapters[chapterIndex].testChamberDisplayNumber);

    chapterMenu->testChamberText = menuBuildPrerenderedText(&gDejaVuSansFont, chapterText, chapterMenu->x, chapterMenu->y + 14, 100);

    if (chapter->imageData) {
        romCopy(chapter->imageData, chapterMenu->imageBuffer, CHAPTER_IMAGE_SIZE);
    }

    int x = chapterMenu->x;
    int y = chapterMenu->testChamberText->y + chapterMenu->testChamberText->height;

    Gfx* gfx = menuRerenderSolidBorder(
        x, y + 4, 
        92, 58,
        x + 5, y + 9,
        CHAPTER_IMAGE_WIDTH, CHAPTER_IMAGE_HEIGHT,
        chapterMenu->border
    );
    gSPEndDisplayList(gfx);

    chapterMenu->chapter = chapter;
}

#define NEW_GAME_LEFT       40
#define NEW_GAME_TOP        45

void newGameInit(struct NewGameMenu* newGameMenu) {
    newGameMenu->menuOutline = menuBuildBorder(NEW_GAME_LEFT, NEW_GAME_TOP, SCREEN_WD - NEW_GAME_LEFT * 2, SCREEN_HT - NEW_GAME_TOP * 2);

    newGameMenu->newGameText = menuBuildPrerenderedText(&gDejaVuSansFont, translationsGet(GAMEUI_NEWGAME), 48, 48, SCREEN_WD);
    
    newGameMenu->topLine = menuBuildHorizontalLine(52, 64, 214);

    chapterMenuInit(&newGameMenu->chapter0, 55, 76);
    chapterMenuInit(&newGameMenu->chapter1, 163, 76);

    chapterMenuSetChapter(&newGameMenu->chapter0, &gChapters[0], 0);
    chapterMenuSetChapter(&newGameMenu->chapter1, &gChapters[1], 1);

    newGameMenu->chapterOffset = 0;
    newGameMenu->selectedChapter = 0;    

    for (
        newGameMenu->chapterCount = 1; 
        newGameMenu->chapterCount < MAX_CHAPTER_COUNT && 
            gChapters[newGameMenu->chapterCount].testChamberNumber <= gSaveData.header.chapterProgress; 
        ++newGameMenu->chapterCount) {
        // do nothing
    }
}

void newGameRebuildText(struct NewGameMenu* newGameMenu) {
    chapterMenuSetChapter(&newGameMenu->chapter0, &gChapters[newGameMenu->chapterOffset], newGameMenu->chapterOffset);
    chapterMenuSetChapter(&newGameMenu->chapter1, &gChapters[newGameMenu->chapterOffset + 1], newGameMenu->chapterOffset + 1);

    prerenderedTextFree(newGameMenu->newGameText);
    newGameMenu->newGameText = menuBuildPrerenderedText(&gDejaVuSansFont, translationsGet(GAMEUI_NEWGAME), 48, 48, SCREEN_WD);
}

enum InputCapture newGameUpdate(struct NewGameMenu* newGameMenu) {
    if (controllerGetButtonDown(0, B_BUTTON)) {
        return InputCaptureExit;
    }

    if (controllerGetButtonDown(0, A_BUTTON) && gChapters[newGameMenu->selectedChapter].testChamberNumber >= 0) {
        gCurrentTestSubject = savefileNextTestSubject();
        soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
        levelQueueLoad(gChapters[newGameMenu->selectedChapter].testChamberNumber, NULL, NULL);
    }

    if ((controllerGetDirectionDown(0) & ControllerDirectionRight) != 0 && 
        newGameMenu->selectedChapter + 1 < newGameMenu->chapterCount &&
        gChapters[newGameMenu->selectedChapter + 1].imageData) {
        newGameMenu->selectedChapter = newGameMenu->selectedChapter + 1;
    }

    if ((controllerGetDirectionDown(0) & ControllerDirectionLeft) != 0 && newGameMenu->selectedChapter > 0) {
        newGameMenu->selectedChapter = newGameMenu->selectedChapter - 1;
    }
    
    if ((controllerGetDirectionDown(0) & ControllerDirectionLeft) != 0 || (controllerGetDirectionDown(0) & ControllerDirectionRight) != 0)
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);

    int nextChapterOffset = newGameMenu->selectedChapter & ~1;

    if (nextChapterOffset != newGameMenu->chapterOffset) {
        newGameMenu->chapterOffset = nextChapterOffset;

        chapterMenuSetChapter(&newGameMenu->chapter0, &gChapters[newGameMenu->chapterOffset], newGameMenu->chapterOffset);
        chapterMenuSetChapter(&newGameMenu->chapter1, &gChapters[newGameMenu->chapterOffset + 1], newGameMenu->chapterOffset + 1);
    }

    return InputCapturePass;
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

    gDPPipeSync(renderState->dl++);
    menuSetRenderColor(renderState, newGameMenu->selectedChapter == newGameMenu->chapterOffset, &gSelectionOrange, &gColorBlack);
    gSPDisplayList(renderState->dl++, newGameMenu->chapter0.border);

    int showSecondChapter = newGameMenu->chapter1.chapter->imageData && newGameMenu->chapter1.chapter->testChamberNumber <= gSaveData.header.chapterProgress;

    if (showSecondChapter) {
        gDPPipeSync(renderState->dl++);
        menuSetRenderColor(renderState, newGameMenu->selectedChapter != newGameMenu->chapterOffset, &gSelectionOrange, &gColorBlack);
        gSPDisplayList(renderState->dl++, newGameMenu->chapter1.border);
    }

    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_ENV_INDEX]);

    struct PrerenderedTextBatch* batch = prerenderedBatchStart();

    prerenderedBatchAdd(batch, newGameMenu->newGameText, NULL);

    prerenderedBatchAdd(batch, newGameMenu->chapter0.chapterText, newGameMenu->selectedChapter == newGameMenu->chapterOffset ? &gSelectionOrange : &gColorWhite);
    prerenderedBatchAdd(batch, newGameMenu->chapter0.testChamberText, newGameMenu->selectedChapter == newGameMenu->chapterOffset ? &gSelectionOrange : &gColorWhite);

    if (showSecondChapter) {
        prerenderedBatchAdd(batch, newGameMenu->chapter1.chapterText, newGameMenu->selectedChapter != newGameMenu->chapterOffset ? &gSelectionOrange : &gColorWhite);
        prerenderedBatchAdd(batch, newGameMenu->chapter1.testChamberText, newGameMenu->selectedChapter != newGameMenu->chapterOffset ? &gSelectionOrange : &gColorWhite);
    }

    renderState->dl = prerenderedBatchFinish(batch, gDejaVuSansImages, renderState->dl);

    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_0_INDEX]);

    graphicsCopyImage(
        renderState, newGameMenu->chapter0.imageBuffer, 
        84, 48, 
        0, 0, 
        newGameMenu->chapter0.x + 5,
        newGameMenu->chapter0.testChamberText->y + newGameMenu->chapter0.testChamberText->height + 9,
        84, 48,
        gColorWhite
    );

    if (showSecondChapter) {
        graphicsCopyImage(
            renderState, newGameMenu->chapter1.imageBuffer, 
            84, 48, 
            0, 0, 
            newGameMenu->chapter1.x + 5,
            newGameMenu->chapter1.testChamberText->y + newGameMenu->chapter1.testChamberText->height + 9,
            84, 48,
            gColorWhite
        );
    }
}
