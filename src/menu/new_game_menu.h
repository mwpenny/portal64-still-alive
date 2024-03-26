#ifndef __MENU_NEW_GAME_MENU_H__
#define __MENU_NEW_GAME_MENU_H__

#include "../graphics/graphics.h"
#include "./menu.h"
#include "../font/font.h"
#include "./confirmation_dialog.h"
#define CHAPTER_IMAGE_WIDTH     84
#define CHAPTER_IMAGE_HEIGHT    48

#define CHAPTER_IMAGE_SIZE  (CHAPTER_IMAGE_WIDTH * CHAPTER_IMAGE_HEIGHT * 2)

struct Chapter {
    void* imageData;
    short testChamberLevelIndex;
    short testChamberDisplayNumber;
};

struct ChapterMenu {
    struct PrerenderedText* chapterText;
    struct PrerenderedText* testChamberText;
    Gfx* border;
    void* imageBuffer;
    struct Chapter* chapter;
    int x;
    int y;
};

void chapterMenuInit(struct ChapterMenu* chapterMenu, int x, int y);

struct NewGameMenu {
    Gfx* menuOutline;
    struct PrerenderedText* newGameText;
    Gfx* topLine;

    struct ConfirmationDialog confirmationDialog;

    struct ChapterMenu displayedChapter0_left;
    struct ChapterMenu displayedChapter1_right;

    short selectedChapter;
    short chapterOffset;
    short chapterCount;
};

void newGameInit(struct NewGameMenu* newGameMenu);
void newGameRebuildText(struct NewGameMenu* newGameMenu);
enum InputCapture newGameUpdate(struct NewGameMenu* newGameMenu);
void newGameRender(struct NewGameMenu* newGameMenu, struct RenderState* renderState, struct GraphicsTask* task);

#endif