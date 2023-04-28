#ifndef __MENU_MENU_H__
#define __MENU_MENU_H__

#include <ultra64.h>
#include "../font/font.h"
#include "../graphics/color.h"
#include "../graphics/graphics.h"

struct MenuButton {
    Gfx* outline;
    Gfx* text;
    short x, y;
    short w, h;
};

enum MenuDirection {
    MenuDirectionStay,
    MenuDirectionUp,
    MenuDirectionRight,
    MenuDirectionLeft,
};

#define GFX_ENTRIES_PER_IMAGE   3
#define GFX_ENTRIES_PER_END_DL  1

extern struct Coloru8 gSelectionOrange;
extern struct Coloru8 gSelectionGray;

extern struct Coloru8 gBorderHighlight;
extern struct Coloru8 gBorderDark;

Gfx* menuBuildText(struct Font* font, char* message, int x, int y);
Gfx* menuBuildBorder(int x, int y, int width, int height);
Gfx* menuBuildHorizontalLine(int x, int y, int width);
Gfx* menuBuildSolidBorder(int x, int y, int w, int h, int nx, int ny, int nw, int nh);
Gfx* menuBuildOutline(int x, int y, int width, int height, int invert);

struct MenuButton menuBuildButton(struct Font* font, char* message, int x, int y, int width, int height);
void menuSetRenderColor(struct RenderState* renderState, int isSelected, struct Coloru8* selected, struct Coloru8* defaultColor);

#endif