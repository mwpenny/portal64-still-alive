#ifndef __MENU_MENU_H__
#define __MENU_MENU_H__

#include <ultra64.h>
#include "../font/font.h"

struct MenuButton {
    Gfx* outline;
    Gfx* text;
};

Gfx* menuBuildText(struct Font* font, char* message, int x, int y);
Gfx* menuBuildBorder(int x, int y, int width, int height);
Gfx* menuBuildHorizontalLine(int x, int y, int width);
Gfx* menuBuildSolidBorder(int x, int y, int w, int h, int nx, int ny, int nw, int nh);

struct MenuButton menuBuildButton(struct Font* font, char* message, int x, int y, int width, int height);

#endif