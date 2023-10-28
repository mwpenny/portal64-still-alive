#ifndef __FONT_FONT_H__
#define __FONT_FONT_H__

#include <ultra64.h>
#include "../math/vector2s16.h"

struct FontKerning {
    char amount;
    short first;
    short second;
};

struct FontSymbol {
    short id;
    char x, y;
    char width, height;
    char xoffset, yoffset;
    char xadvance;
    char textureIndex;
};

struct Font {
    struct FontKerning* kerning;
    struct FontSymbol* symbols;
    Gfx* images;

    char base;
    char charHeight;
    unsigned short symbolMultiplier;
    unsigned short symbolMask;
    unsigned short symbolMaxCollisions;

    unsigned short kerningMultiplier;
    unsigned short kerningMask;
    unsigned short kerningMaxCollisions;
};

struct SymbolLocation {
    short x;
    short y;
    short symbolIndex;
};

Gfx* fontRender(struct Font* font, char* message, int x, int y, Gfx* dl);
int fontCountGfx(struct Font* font, char* message);
struct Vector2s16 fontMeasure(struct Font* font, char* message);

struct FontRenderer {
    struct SymbolLocation symbols[128];
    short currentSymbol;
};

void fontRendererRender(struct FontRenderer* renderer, struct Font* font, char* message, int x, int y, int maxWidth);
Gfx* fontRendererBuildGfx(struct FontRenderer* renderer, struct Font* font, Gfx* gfx);

#endif