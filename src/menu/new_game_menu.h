#ifndef __MENU_NEW_GAME_MENU_H__
#define __MENU_NEW_GAME_MENU_H__

#include "confirmation_dialog.h"
#include "font/font.h"
#include "graphics/graphics.h"
#include "menu.h"

#define CHAPTER_IMAGE_WIDTH     84
#define CHAPTER_IMAGE_HEIGHT    48

#define CHAPTER_IMAGE_SIZE      (CHAPTER_IMAGE_WIDTH * CHAPTER_IMAGE_HEIGHT * 2)

struct Chapter {
    void* imageData;
    short testChamberLevelIndex;
    short testChamberNumber;
};

struct ChapterMenuItem {
    struct PrerenderedText* chapterText;
    struct PrerenderedText* testChamberText;
    Gfx* border;
    void* imageBuffer;
    struct Chapter* chapter;
    int x;
    int y;
};

struct NewGameMenu {
    struct PrerenderedText* newGameText;
    Gfx* menuOutline;
    Gfx* topLine;

    struct ConfirmationDialog confirmationDialog;

    struct ChapterMenuItem leftChapter;
    struct ChapterMenuItem rightChapter;

    short leftChapterIndex;
    short selectedChapterIndex;
    short chapterCount;
};

void newGameInit(struct NewGameMenu* newGameMenu);
void newGameRebuildText(struct NewGameMenu* newGameMenu);
enum InputCapture newGameUpdate(struct NewGameMenu* newGameMenu);
void newGameRender(struct NewGameMenu* newGameMenu, struct RenderState* renderState, struct GraphicsTask* task);

#endif