#ifndef __MENU_NEW_GAME_MENU_H__
#define __MENU_NEW_GAME_MENU_H__

#include "../graphics/graphics.h"
#include "./menu.h"
#define CHAPTER_IMAGE_WIDTH     84
#define CHAPTER_IMAGE_HEIGHT    48

#define CHAPTER_IMAGE_SIZE  (CHAPTER_IMAGE_WIDTH * CHAPTER_IMAGE_HEIGHT * 2)

struct Chapter {
    char* chapter;
    char* testChamber;
    void* imageData;
    int testChamberNumber;
};

struct ChapterMenu {
    Gfx* chapterText;
    Gfx* testChamberText;
    Gfx* border;
    void* imageBuffer;
    struct Chapter* chapter;
    int x;
    int y;
};

struct Chapter* chapterFindForChamber(int chamberIndex);

void chapterMenuInit(struct ChapterMenu* chapterMenu, int x, int y);
void chapterMenuSetChapter(struct ChapterMenu* chapterMenu, struct Chapter* chapter);

struct NewGameMenu {
    Gfx* menuOutline;
    Gfx* newGameText;
    Gfx* topLine;

    struct ChapterMenu chapter0;
    struct ChapterMenu chapter1;

    short selectedChapter;
    short chapterOffset;
    short chapterCount;
};

void newGameInit(struct NewGameMenu* newGameMenu);
enum MenuDirection newGameUpdate(struct NewGameMenu* newGameMenu);
void newGameRender(struct NewGameMenu* newGameMenu, struct RenderState* renderState, struct GraphicsTask* task);

#endif