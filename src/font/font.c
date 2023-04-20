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