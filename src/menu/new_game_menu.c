#include "new_game_menu.h"

#include "../font/font.h"
#include "../font/dejavusans.h"

#include "../graphics/image.h"
#include "../util/memory.h"
#include "../util/rom.h"
#include "../audio/soundplayer.h"

#include "../build/assets/materials/ui.h"
#include "../build/assets/materials/images.h"
#include "../build/src/audio/clips.h"

#include "../levels/levels.h"
#include "../savefile/savefile.h"
#include "../controls/controller.h"

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
    chapterMenu->chapterText = malloc(sizeof(Gfx) * GFX_ENTRIES_PER_IMAGE * 10 + GFX_ENTRIES_PER_END_DL);
    chapterMenu->testChamberText = malloc(sizeof(Gfx) * GFX_ENTRIES_PER_IMAGE * 14 + GFX_ENTRIES_PER_END_DL);
    chapterMenu->border = menuBuildSolidBorder(
        x, y + 27, 
        92, 58,
        x + 5, y + 32,
        CHAPTER_IMAGE_WIDTH, CHAPTER_IMAGE_HEIGHT
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

    chapterMenuInit(&newGameMenu->chapter0, 55, 76);
    chapterMenuInit(&newGameMenu->chapter1, 163, 76);

    chapterMenuSetChapter(&newGameMenu->chapter0, &gChapters[0]);
    chapterMenuSetChapter(&newGameMenu->chapter1, &gChapters[1]);

    newGameMenu->chapterOffset = 0;
    newGameMenu->selectedChapter = 0;    

    for (
        newGameMenu->chapterCount = 1; 
        newGameMenu->chapterCount < MAX_CHAPTER_COUNT && 
            gChapters[newGameMenu->chapterCount].testChamberNumber <= gSaveData.header.chapterProgress; 
        ++newGameMenu->chapterCount) {

    }
}

enum MenuDirection newGameUpdate(struct NewGameMenu* newGameMenu) {
    if (controllerGetButtonDown(0, B_BUTTON)) {
        return MenuDirectionUp;
    }

    if (controllerGetButtonDown(0, A_BUTTON) && gChapters[newGameMenu->selectedChapter].testChamberNumber >= 0) {
        gCurrentTestSubject = savefileNextTestSubject();
        soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL);
        levelQueueLoad(gChapters[newGameMenu->selectedChapter].testChamberNumber, NULL, NULL);
    }

    if ((controllerGetDirectionDown(0) & ControllerDirectionRight) != 0 && 
        newGameMenu->selectedChapter + 1 < newGameMenu->chapterCount &&
        gChapters[newGameMenu->selectedChapter + 1].imageData) {
        newGameMenu->selectedChapter = newGameMenu->selectedChapter + 1;
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 0.5f, NULL, NULL);
    }

    if ((controllerGetDirectionDown(0) & ControllerDirectionLeft) != 0 && newGameMenu->selectedChapter > 0) {
        newGameMenu->selectedChapter = newGameMenu->selectedChapter - 1;
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 0.5f, NULL, NULL);
    }

    int nextChapterOffset = newGameMenu->selectedChapter & ~1;

    if (nextChapterOffset != newGameMenu->chapterOffset) {
        newGameMenu->chapterOffset = nextChapterOffset;

        chapterMenuSetChapter(&newGameMenu->chapter0, &gChapters[newGameMenu->chapterOffset]);
        chapterMenuSetChapter(&newGameMenu->chapter1, &gChapters[newGameMenu->chapterOffset + 1]);
    }

    return MenuDirectionStay;
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

    gSPDisplayList(renderState->dl++, ui_material_list[DEJAVU_SANS_INDEX]);
    gSPDisplayList(renderState->dl++, newGameMenu->newGameText);

    gDPPipeSync(renderState->dl++);
    menuSetRenderColor(renderState, newGameMenu->selectedChapter == newGameMenu->chapterOffset, &gSelectionOrange, &gColorWhite);
    gSPDisplayList(renderState->dl++, newGameMenu->chapter0.chapterText);
    gSPDisplayList(renderState->dl++, newGameMenu->chapter0.testChamberText);

    if (showSecondChapter) {
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

    if (showSecondChapter) {
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