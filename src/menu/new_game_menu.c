#include "new_game_menu.h"

#include "font/font.h"
#include "font/dejavu_sans.h"

#include "audio/soundplayer.h"
#include "graphics/image.h"
#include "levels/levels.h"
#include "strings/translations.h"
#include "savefile/savefile.h"
#include "system/cartridge.h"
#include "system/controller.h"
#include "system/screen.h"
#include "text_manipulation.h"
#include "util/memory.h"

#include "codegen/assets/audio/clips.h"
#include "codegen/assets/materials/ui.h"
#include "codegen/assets/materials/images.h"
#include "codegen/assets/strings/strings.h"

struct Chapter gChapters[] = {
    {images_chapter1_rgba_16b, 0, 0},
    {images_chapter2_rgba_16b, 2, 4},
    {images_chapter3_rgba_16b, 4, 8},
    {images_chapter4_rgba_16b, 6, 10},
    {images_chapter5_rgba_16b, 8, 13},
    {images_chapter6_rgba_16b, 9, 14},
    {images_chapter7_rgba_16b, 10, 15},
    {images_chapter8_rgba_16b, 11, 16},
    {images_chapter9_rgba_16b, -1, 17},
    {images_chapter10_rgba_16b, -1, 18},
    {images_chapter11_rgba_16b, -1, 19},
};

#define TOTAL_CHAPTER_COUNT ((sizeof(gChapters) / sizeof(*gChapters)))
#define NEW_GAME_X          40
#define NEW_GAME_Y          45

static void chapterMenuItemInit(struct ChapterMenuItem* chapterMenuItem, int x, int y) {
    chapterMenuItem->chapterText = NULL;
    chapterMenuItem->testChamberText = NULL;
    chapterMenuItem->border = menuBuildSolidBorder(
        x, y + 27,
        92, 58,
        x + 5, y + 32,
        CHAPTER_IMAGE_WIDTH, CHAPTER_IMAGE_HEIGHT
    );

    chapterMenuItem->imageBuffer = malloc(CHAPTER_IMAGE_SIZE);
    zeroMemory(chapterMenuItem->imageBuffer, CHAPTER_IMAGE_SIZE);

    chapterMenuItem->chapter = NULL;
    chapterMenuItem->x = x;
    chapterMenuItem->y = y;
}

static void chapterMenuItemSetChapter(struct ChapterMenuItem* chapterMenuItem, int chapterIndex) {
    struct Chapter* chapter = &gChapters[chapterIndex];

    menuFreePrerenderedDeferred(chapterMenuItem->chapterText);
    menuFreePrerenderedDeferred(chapterMenuItem->testChamberText);

    char chapterText[64];
    sprintf(chapterText, "%s %d", translationsGet(GAMEUI_CHAPTER), chapterIndex + 1);
    chapterMenuItem->chapterText = menuBuildPrerenderedText(&gDejaVuSansFont, chapterText, chapterMenuItem->x, chapterMenuItem->y, SCREEN_WD);

    textManipTestChamberMessage(chapterText, gChapters[chapterIndex].testChamberNumber);
    chapterMenuItem->testChamberText = menuBuildPrerenderedText(&gDejaVuSansFont, chapterText, chapterMenuItem->x, chapterMenuItem->y + 14, 100);

    if (chapter->testChamberLevelIndex >= 0) {
        romCopy(chapter->imageData, chapterMenuItem->imageBuffer, CHAPTER_IMAGE_SIZE);
    }

    int x = chapterMenuItem->x;
    int y = chapterMenuItem->testChamberText->y + chapterMenuItem->testChamberText->height;

    Gfx* gfx = menuRerenderSolidBorder(
        x, y + 4,
        92, 58,
        x + 5, y + 9,
        CHAPTER_IMAGE_WIDTH, CHAPTER_IMAGE_HEIGHT,
        chapterMenuItem->border
    );
    gSPEndDisplayList(gfx);

    chapterMenuItem->chapter = chapter;
}

void newGameInit(struct NewGameMenu* newGameMenu) {
    newGameMenu->newGameText = menuBuildPrerenderedText(&gDejaVuSansFont, translationsGet(GAMEUI_NEWGAME), 48, 48, SCREEN_WD);
    newGameMenu->menuOutline = menuBuildBorder(NEW_GAME_X, NEW_GAME_Y, SCREEN_WD - (NEW_GAME_X * 2), SCREEN_HT - (NEW_GAME_Y) * 2);
    newGameMenu->topLine = menuBuildHorizontalLine(52, 64, 214);

    confirmationDialogInit(&newGameMenu->confirmationDialog);

    chapterMenuItemInit(&newGameMenu->leftChapter, 55, 76);
    chapterMenuItemInit(&newGameMenu->rightChapter, 163, 76);

    chapterMenuItemSetChapter(&newGameMenu->leftChapter, 0);
    chapterMenuItemSetChapter(&newGameMenu->rightChapter, 1);

    newGameMenu->leftChapterIndex = 0;
    newGameMenu->selectedChapterIndex = 0;
    newGameMenu->chapterCount = 1;
}

void newGameRebuildText(struct NewGameMenu* newGameMenu) {
    chapterMenuItemSetChapter(&newGameMenu->leftChapter, newGameMenu->leftChapterIndex);

    if ((newGameMenu->leftChapterIndex + 1) < TOTAL_CHAPTER_COUNT) {
        chapterMenuItemSetChapter(&newGameMenu->rightChapter, newGameMenu->leftChapterIndex + 1);
    }

    prerenderedTextFree(newGameMenu->newGameText);
    newGameMenu->newGameText = menuBuildPrerenderedText(&gDejaVuSansFont, translationsGet(GAMEUI_NEWGAME), 48, 48, SCREEN_WD);
}

static void newGameStartSelectedChapter(struct NewGameMenu* newGameMenu) {
    gCurrentTestSubject = savefileNextTestSubject();
    levelQueueLoad(
        gChapters[newGameMenu->selectedChapterIndex].testChamberLevelIndex,
        NULL,
        NULL,
        0 /* useCheckpoint */
    );
}

static void newGameConfirmStartClosed(struct NewGameMenu* newGameMenu, int isConfirmed) {
    if (isConfirmed) {
        newGameStartSelectedChapter(newGameMenu);
    }
}

enum InputCapture newGameUpdate(struct NewGameMenu* newGameMenu) {
    if (newGameMenu->confirmationDialog.isShown) {
        return confirmationDialogUpdate(&newGameMenu->confirmationDialog);
    }

    // This is done on update so if the unlock menu cheat is used it shows up right away
    while (newGameMenu->chapterCount < TOTAL_CHAPTER_COUNT &&
            gChapters[newGameMenu->chapterCount].testChamberLevelIndex <= gSaveData.header.chapterProgressLevelIndex &&
            gChapters[newGameMenu->chapterCount].testChamberLevelIndex > 0) {
        ++newGameMenu->chapterCount;
    }

    if (controllerGetButtonsDown(0, ControllerButtonB)) {
        return InputCaptureExit;
    }

    if (controllerGetButtonsDown(0, ControllerButtonA)) {
        if (gScene.mainMenuMode) {
            newGameStartSelectedChapter(newGameMenu);
        } else {
            struct ConfirmationDialogParams dialogParams = {
                translationsGet(GAMEUI_CONFIRMNEWGAME_TITLE),
                translationsGet(GAMEUI_NEWGAMEWARNING),
                translationsGet(GAMEUI_YES),
                translationsGet(GAMEUI_NO),
                0,
                (ConfirmationDialogCallback)&newGameConfirmStartClosed,
                newGameMenu
            };

            confirmationDialogShow(&newGameMenu->confirmationDialog, &dialogParams);
        }
        soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);
    }

    int controllerDir = controllerGetDirectionDown(0);

    if (controllerDir & ControllerDirectionRight) {
        if ((newGameMenu->selectedChapterIndex + 1) < newGameMenu->chapterCount)
            ++newGameMenu->selectedChapterIndex;
        else
            newGameMenu->selectedChapterIndex = 0;
    }

    if (controllerDir & ControllerDirectionLeft) {
        if (newGameMenu->selectedChapterIndex > 0)
            --newGameMenu->selectedChapterIndex;
        else
            newGameMenu->selectedChapterIndex = newGameMenu->chapterCount - 1;
    }

    if (newGameMenu->chapterCount > 1 && (controllerDir & (ControllerDirectionLeft | ControllerDirectionRight))) {
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);
    }

    int nextLeftChapterIndex = newGameMenu->selectedChapterIndex & ~1;

    if (nextLeftChapterIndex != newGameMenu->leftChapterIndex) {
        newGameMenu->leftChapterIndex = nextLeftChapterIndex;

        chapterMenuItemSetChapter(&newGameMenu->leftChapter, newGameMenu->leftChapterIndex);

        if ((newGameMenu->leftChapterIndex + 1) < TOTAL_CHAPTER_COUNT) {
            chapterMenuItemSetChapter(&newGameMenu->rightChapter, newGameMenu->leftChapterIndex + 1);
        }
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

    int leftChapterSelected = newGameMenu->selectedChapterIndex == newGameMenu->leftChapterIndex;
    int showRightChapter = (newGameMenu->leftChapterIndex + 1) < TOTAL_CHAPTER_COUNT &&
        newGameMenu->rightChapter.chapter->testChamberLevelIndex <= gSaveData.header.chapterProgressLevelIndex &&
        newGameMenu->rightChapter.chapter->testChamberLevelIndex > 0;

    gDPPipeSync(renderState->dl++);
    menuSetRenderColor(renderState, leftChapterSelected, &gSelectionOrange, &gColorBlack);
    gSPDisplayList(renderState->dl++, newGameMenu->leftChapter.border);

    if (showRightChapter) {
        gDPPipeSync(renderState->dl++);
        menuSetRenderColor(renderState, !leftChapterSelected, &gSelectionOrange, &gColorBlack);
        gSPDisplayList(renderState->dl++, newGameMenu->rightChapter.border);
    }

    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_ENV_INDEX]);

    struct PrerenderedTextBatch* batch = prerenderedBatchStart();

    prerenderedBatchAdd(batch, newGameMenu->newGameText, NULL);

    prerenderedBatchAdd(batch, newGameMenu->leftChapter.chapterText, leftChapterSelected ? &gSelectionOrange : &gColorWhite);
    prerenderedBatchAdd(batch, newGameMenu->leftChapter.testChamberText, leftChapterSelected ? &gSelectionOrange : &gColorWhite);

    if (showRightChapter) {
        prerenderedBatchAdd(batch, newGameMenu->rightChapter.chapterText, !leftChapterSelected ? &gSelectionOrange : &gColorWhite);
        prerenderedBatchAdd(batch, newGameMenu->rightChapter.testChamberText, !leftChapterSelected ? &gSelectionOrange : &gColorWhite);
    }

    renderState->dl = prerenderedBatchFinish(batch, gDejaVuSansImages, renderState->dl);

    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_0_INDEX]);

    graphicsCopyImage(
        renderState, newGameMenu->leftChapter.imageBuffer,
        84, 48,
        0, 0,
        newGameMenu->leftChapter.x + 5,
        newGameMenu->leftChapter.testChamberText->y + newGameMenu->leftChapter.testChamberText->height + 9,
        84, 48,
        gColorWhite
    );

    if (showRightChapter) {
        graphicsCopyImage(
            renderState, newGameMenu->rightChapter.imageBuffer,
            84, 48,
            0, 0,
            newGameMenu->rightChapter.x + 5,
            newGameMenu->rightChapter.testChamberText->y + newGameMenu->rightChapter.testChamberText->height + 9,
            84, 48,
            gColorWhite
        );
    }

    if (newGameMenu->confirmationDialog.isShown) {
        confirmationDialogRender(&newGameMenu->confirmationDialog, renderState);
    }
}
