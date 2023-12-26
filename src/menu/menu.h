#ifndef __MENU_MENU_H__
#define __MENU_MENU_H__

#include <ultra64.h>
#include "../font/font.h"
#include "../graphics/color.h"
#include "../graphics/graphics.h"

#define CHECKBOX_SIZE   12

struct MenuButton {
    Gfx* outline;
    struct PrerenderedText* text;
    short x, y;
    short w, h;
};

struct MenuCheckbox {
    Gfx* outline;
    struct PrerenderedText* prerenderedText;
    Gfx* checkedIndicator;
    short x, y;
    short checked;
};

struct MenuSlider {
    Gfx* back;
    float value;
    short x, y;
    short w;
};

enum InputCapture {
    InputCapturePass,
    InputCaptureGrab,
    InputCaptureExit,
};

#define GFX_ENTRIES_PER_IMAGE   3
#define GFX_ENTRIES_PER_END_DL  1

extern struct Coloru8 gSelectionOrange;
extern struct Coloru8 gSelectionGray;

extern struct Coloru8 gBorderHighlight;
extern struct Coloru8 gBorderDark;

struct PrerenderedText* menuBuildPrerenderedText(struct Font* font, char* message, int x, int y, int maxWidth);

Gfx* menuRerenderBorder(int x, int y, int width, int height, Gfx* dl);
Gfx* menuBuildBorder(int x, int y, int width, int height);
Gfx* menuBuildHorizontalLine(int x, int y, int width);
Gfx* menuRerenderSolidBorder(int x, int y, int w, int h, int nx, int ny, int nw, int nh, Gfx* dl);
Gfx* menuBuildSolidBorder(int x, int y, int w, int h, int nx, int ny, int nw, int nh);
Gfx* menuBuildOutline(int x, int y, int width, int height, int invert);

struct MenuButton menuBuildButton(struct Font* font, char* message, int x, int y, int height, int rightAlign);
void menuSetRenderColor(struct RenderState* renderState, int isSelected, struct Coloru8* selected, struct Coloru8* defaultColor);
void menuRebuildButtonText(struct MenuButton* button, struct Font* font, char* message, int rightAlign);
void menuRelocateButton(struct MenuButton* button, int x, int y, int rightAlign);

struct MenuCheckbox menuBuildCheckbox(struct Font* font, char* message, int x, int y);
Gfx* menuCheckboxRender(struct MenuCheckbox* checkbox, Gfx* dl);

struct MenuSlider menuBuildSlider(int x, int y, int w, int tickCount);
Gfx* menuSliderRender(struct MenuSlider* slider, Gfx* dl);

void menuFreePrerenderedDeferred(struct PrerenderedText* text);
void menuTickDeferredQueue();
void menuResetDeferredQueue();

#endif