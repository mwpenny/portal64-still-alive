#ifndef __FONT_FONT_H__
#define __FONT_FONT_H__

#include <ultra64.h>
#include "../math/vector2s16.h"

struct FontKerning {
    char amount;
    char first;
    char second;
};

struct FontSymbol {
    char x, y;
    char width, height;
    char xoffset, yoffset;
    char xadvance;
};

struct Font {
    struct FontKerning* kerning;
    struct FontSymbol* symbols;

    char base;
    char charHeight;
    unsigned short symbolCount;
    unsigned short kerningMultiplier;
    unsigned short kerningMask;
    unsigned short maxCollisions;
};

int fontDetermineKerning(struct Font* font, char first, char second);
Gfx* fontRender(struct Font* font, char* message, int x, int y, Gfx* dl);
int fontCountGfx(struct Font* font, char* message);
struct Vector2s16 fontMeasure(struct Font* font, char* message);

#endif