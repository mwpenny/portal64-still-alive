#ifndef __MENU_NEW_GAME_MENU_H__
#define __MENU_NEW_GAME_MENU_H__

#include "../graphics/graphics.h"
#include "./menu.h"
#include "./menu_state.h"

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
};

void newGameInit(struct NewGameMenu* newGameMenu);
enum MainMenuState newGameUpdate(struct NewGameMenu* newGameMenu);
void newGameRender(struct NewGameMenu* newGameMenu, struct RenderState* renderState, struct GraphicsTask* task);

#endif