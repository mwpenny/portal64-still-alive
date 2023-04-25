#include "font.h"

int fontDetermineKerning(struct Font* font, char first, char second) {
    unsigned index = ((unsigned)first * (unsigned)font->kerningMultiplier + (unsigned)second) & (unsigned)font->kerningMask;
    int maxIterations = font->maxCollisions;

    do {
        struct FontKerning* kerning = &font->kerning[index];

        if (kerning->amount == 0) {
            return 0;
        }

        if (kerning->first == first && kerning->second == second) {
            return kerning->amount;
        }
        
        ++index;
        --maxIterations;
    } while (maxIterations >= 0);

    return 0;
}

Gfx* fontRender(struct Font* font, char* message, int x, int y, Gfx* dl) {
    int startX = x;
    char prev = 0;

    for (; *message; prev = *message, ++message) {
        char curr = *message;

        if (curr == '\n') {
            y += font->charHeight;
            x = startX;
            continue;
        }

        if ((unsigned char)curr >= font->symbolCount) {
            continue;
        }

        struct FontSymbol* symbol = &font->symbols[(int)curr];

        x += fontDetermineKerning(font, prev, curr);

        int finalX = x + symbol->xoffset;
        int finalY = y + symbol->yoffset;

        gSPTextureRectangle(
            dl++, 
            finalX << 2, finalY << 2,
            (finalX + symbol->width) << 2,
            (finalY + symbol->height) << 2,
            G_TX_RENDERTILE,
            symbol->x << 5, symbol->y << 5,
            0x400, 0x400
        );

        x += symbol->xadvance;
    }

    return dl;
}

int fontCountGfx(struct Font* font, char* message) {
    int result = 0;

    for (; *message; ++message) {
        char curr = *message;

        if (curr == '\n') {
            continue;
        }

        if ((unsigned char)curr >= font->symbolCount) {
            continue;
        }

        result += 3;
    }

    return result;
}

struct Vector2s16 fontMeasure(struct Font* font, char* message) {
    int startX = 0;
    char prev = 0;
    int x = 0;
    int y = 0;

    struct Vector2s16 result;

    result.x = 0;

    for (; *message; prev = *message, ++message) {
        char curr = *message;

        if (curr == '\n') {
            y += font->charHeight;
            x = startX;
            continue;
        }

        if ((unsigned char)curr >= font->symbolCount) {
            continue;
        }

        struct FontSymbol* symbol = &font->symbols[(int)curr];

        x += fontDetermineKerning(font, prev, curr);
        x += symbol->xadvance;

        result.x = MAX(result.x, x);
    }

    result.y = y + font->charHeight;

    return result;
}