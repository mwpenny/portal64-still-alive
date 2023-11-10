#ifndef __FONT_FONT_H__
#define __FONT_FONT_H__

#include <ultra64.h>
#include "../math/vector2s16.h"
#include "../graphics/color.h"

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

    char base;
    char charHeight;
    unsigned short symbolMultiplier;
    unsigned short symbolMask;
    unsigned short symbolMaxCollisions;

    unsigned short kerningMultiplier;
    unsigned short kerningMask;
    unsigned short kerningMaxCollisions;
};

// legacy methods for a font that fits into a single page
Gfx* fontRender(struct Font* font, char* message, int x, int y, Gfx* dl);
int fontCountGfx(struct Font* font, char* message);
struct Vector2s16 fontMeasure(struct Font* font, char* message);

struct SymbolLocation {
    short x;
    short y;
    char sourceX;
    char sourceY;
    char width;
    char height;
    char canBreak;
    char imageIndex;
};

#define FONT_RENDERER_MAX_SYBMOLS   340

struct FontRenderer {
    struct SymbolLocation symbols[FONT_RENDERER_MAX_SYBMOLS];
    short currentSymbol;
    short width;
    short height;
    short usedImageIndices;
};

void fontRendererLayout(struct FontRenderer* renderer, struct Font* font, char* message, int maxWidth);
Gfx* fontRendererBuildGfx(struct FontRenderer* renderer, Gfx** fontImages, int x, int y, struct Coloru8* color, Gfx* gfx);

struct PrerenderedText {
    Gfx** displayLists;
    short usedImageIndices;
    short x;
    short y;
    short width;
    short height;
};

void fontRendererInitPrerender(struct FontRenderer* renderer, struct PrerenderedText* prerender);
struct PrerenderedText* prerenderedTextNew(struct FontRenderer* renderer);
struct PrerenderedText* prerenderedTextCopy(struct PrerenderedText* text);
void prerenderedTextCleanup(struct PrerenderedText* prerender);
void prerenderedTextFree(struct PrerenderedText* prerender);
void prerenderedTextRelocate(struct PrerenderedText* prerender, int x, int y);
void prerenderedTextRecolor(struct PrerenderedText* prerender, struct Coloru8* color);

void fontRendererFillPrerender(struct FontRenderer* renderer, struct PrerenderedText* prerender, int x, int y, struct Coloru8* color);

#define MAX_PRERENDERED_STRINGS     32

struct PrerenderedTextBatch {
    struct PrerenderedText* text[MAX_PRERENDERED_STRINGS];
    unsigned short textCount;
    short usedImageIndices;
};

struct PrerenderedTextBatch* prerenderedBatchStart();
void prerenderedBatchAdd(struct PrerenderedTextBatch* batch, struct PrerenderedText* text, struct Coloru8* color);
Gfx* prerenderedBatchFinish(struct PrerenderedTextBatch* batch, Gfx** fontImages, Gfx* gfx);

#endif