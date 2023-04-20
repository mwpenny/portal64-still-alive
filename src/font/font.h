#ifndef __FONT_FONT_H__
#define __FONT_FONT_H__

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

#endif