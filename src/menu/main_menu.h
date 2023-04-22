#ifndef __MENU_MAIN_MENU_H___
#define __MENU_MAIN_MENU_H___

#include "../graphics/graphics.h"
#include "./menu.h"

struct LandingMenu {
    Gfx* newGameText;
    Gfx* loadGameText;
    Gfx* optionsText;
};

void landingMenuInit(struct LandingMenu* landingMenu);
void landingMenuRender(struct LandingMenu* landingMenu, struct RenderState* renderState, struct GraphicsTask* task);

struct Chapter {
    char* chapter;
    char* testChamber;
    void* imageData;
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
    Gfx* bottomLine;

    struct MenuButton cancelButton;

    struct ChapterMenu chapter0;
    struct ChapterMenu chapter1;
};

void newGameInit(struct NewGameMenu* newGameMenu);
void newGameRender(struct NewGameMenu* newGameMenu, struct RenderState* renderState, struct GraphicsTask* task);

struct MainMenu {
    struct LandingMenu landingMenu;
    struct NewGameMenu newGameMenu;
};

void mainMenuInit(struct MainMenu* mainMenu);
void mainMenuUpdate(struct MainMenu* mainMenu);
void mainMenuRender(struct MainMenu* mainMenu, struct RenderState* renderState, struct GraphicsTask* task);

#endif