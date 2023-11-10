#include "font.h"
#include "../util/memory.h"

#define TEXTURE_IMAGE_INDEX_TO_MASK(index) (1 << (index))

int fontDetermineKerning(struct Font* font, short first, short second) {
    unsigned index = ((unsigned)first * (unsigned)font->kerningMultiplier + (unsigned)second) & (unsigned)font->kerningMask;
    int maxIterations = font->kerningMaxCollisions;

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

struct FontSymbol* fontFindSymbol(struct Font* font, short id) {
    unsigned index = ((unsigned)id * (unsigned)font->symbolMultiplier) & (unsigned)font->symbolMask;
    int maxIterations = font->symbolMaxCollisions;

    do {
        struct FontSymbol* symbol = &font->symbols[index];

        if (symbol->textureIndex == -1) {
            return NULL;
        }

        if (symbol->id == id) {
            return symbol;
        }
        
        ++index;
        --maxIterations;
    } while (maxIterations >= 0);

    return NULL;
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

        // TODO utf-8 decode
        struct FontSymbol* symbol = fontFindSymbol(font, (short)curr);

        if (!symbol) {
            continue;
        }

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

        // TODO utf-8 decode
        struct FontSymbol* symbol = fontFindSymbol(font, (short)curr);

        if (!symbol) {
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

        // TODO utf-8 decode
        struct FontSymbol* symbol = fontFindSymbol(font, (short)curr);

        if (!symbol) {
            continue;
        }

        x += fontDetermineKerning(font, prev, curr);
        x += symbol->xadvance;

        result.x = MAX(result.x, x);
    }

    result.y = y + font->charHeight;

    return result;
}

short fontNextUtf8Character(char** strPtr) {
    char* curr = *strPtr;

    // in the middle of a code point
    // try to find the start of a charcter
    while ((*curr & 0xC0) == 0x80) {
        ++curr;
    }

    if (!(*curr & 0x80)) {
        *strPtr = curr + 1;
        return *curr;
    }

    if ((*curr & 0xE0) == 0xC0) {
        *strPtr = curr + 2;
        return ((short)(curr[0] & 0x1F) << 6) | (short)(curr[1] & 0x3F);

    } else if ((*curr & 0xF0) == 0xE0) {
        *strPtr = curr + 3;
        return ((short)(curr[0] & 0xF) << 12) | ((short)(curr[1] & 0x3F) << 6) | (short)(curr[2] & 0x3F);

    } else if ((*curr & 0xF8) == 0xF0) {
        *strPtr = curr + 4;
        // utf character out of range of a short
        return 0;
    } else {
        // invalid unicode character
        *strPtr = curr + 1;
        return 0;
    }
}

int fontRendererFindBreak(struct FontRenderer* renderer) {
    for (int search = renderer->currentSymbol - 1; search > 0; --search) {
        if (renderer->symbols[search].canBreak) {
            return search + 1;
        }
    }

    return renderer->currentSymbol - 1;
}

void fontRendererWrap(struct FontRenderer* renderer, int from, int xOffset, int yOffset) {
    for (int i = from; i < renderer->currentSymbol; ++i) {
        renderer->symbols[i].x += xOffset;
        renderer->symbols[i].y += yOffset;
    }
}

void fontRendererLayout(struct FontRenderer* renderer, struct Font* font, char* message, int maxWidth) {
    renderer->width = 0;
    renderer->height = 0;
    renderer->currentSymbol = 0;
    renderer->usedImageIndices = 0;

    short prev = 0;
    short curr = 0;
    int x = 0;
    int y = 0;
    int currentMaxWidth = 0;

    while (*message && renderer->currentSymbol < FONT_RENDERER_MAX_SYBMOLS) {
        prev = curr;
        // also advances message to the next character
        curr = fontNextUtf8Character(&message);

        if (curr == '\n') {
            currentMaxWidth = MAX(currentMaxWidth, x);
            y += font->charHeight;
            x = 0;
            continue;
        }

        struct FontSymbol* symbol = fontFindSymbol(font, curr);

        if (!symbol) {
            continue;
        }

        x += fontDetermineKerning(font, prev, curr);

        struct SymbolLocation* target = &renderer->symbols[renderer->currentSymbol];

        target->x = x + symbol->xoffset;
        target->y = y + symbol->yoffset;
        target->width = symbol->width;
        target->height = symbol->height;
        target->canBreak = curr == ' ';
        target->sourceX = symbol->x;
        target->sourceY = symbol->y;
        target->imageIndex = symbol->textureIndex;

        renderer->usedImageIndices |= TEXTURE_IMAGE_INDEX_TO_MASK(symbol->textureIndex);

        ++renderer->currentSymbol;
        x += symbol->xadvance;

        if (x > maxWidth) {
            int breakAt = fontRendererFindBreak(renderer);

            if (breakAt == renderer->currentSymbol) {
                currentMaxWidth = MAX(currentMaxWidth, x);
                y += font->charHeight;
                x = 0;
            } else {
                int lastCharacterX = renderer->symbols[breakAt].x;
                currentMaxWidth = MAX(currentMaxWidth, lastCharacterX);
                fontRendererWrap(renderer, breakAt, -lastCharacterX, font->charHeight);
                y += font->charHeight;
                x -= lastCharacterX;
            }
        }
    }

    renderer->width = MAX(currentMaxWidth, x);
    renderer->height = y + font->charHeight;
}

Gfx* fontRendererBuildSingleGfx(struct FontRenderer* renderer, int imageIndex, int x, int y, Gfx* gfx) {
    for (int i = 0; i < renderer->currentSymbol; ++i) {
        struct SymbolLocation* target = &renderer->symbols[i];

        if (target->imageIndex != imageIndex) {
            continue;
        }

        int finalX = target->x + x;
        int finalY = target->y + y;

        gSPTextureRectangle(
            gfx++, 
            finalX << 2, finalY << 2,
            (finalX + target->width) << 2,
            (finalY + target->height) << 2,
            G_TX_RENDERTILE,
            target->sourceX << 5, target->sourceY << 5,
            0x400, 0x400
        );
    }

    return gfx;
}

Gfx* fontRendererBuildGfx(struct FontRenderer* renderer, Gfx** fontImages, int x, int y, struct Coloru8* color, Gfx* gfx) {
    int imageMask = renderer->usedImageIndices;
    int imageIndex = 0;

    while (imageMask) {
        if (imageMask & 0x1) {
            gSPDisplayList(gfx++, fontImages[imageIndex]);

            if (color) {
                gDPSetEnvColor(gfx++, color->r, color->g, color->b, color->a);
            }

            gfx = fontRendererBuildSingleGfx(renderer, imageIndex, x, y, gfx);
        }

        imageMask >>= 1;
        ++imageIndex;
    }

    return gfx;
}

void fontRendererInitPrerender(struct FontRenderer* renderer, struct PrerenderedText* prerender) {
    int imageIndex = 0;
    int imageMask = renderer->usedImageIndices;

    while (imageMask) {
        imageMask >>= 1;
        ++imageIndex;
    }

    prerender->displayLists = malloc(sizeof(Gfx*) * imageIndex);

    prerender->usedImageIndices = renderer->usedImageIndices;
    prerender->x = 0;
    prerender->y = 0;

    imageMask = renderer->usedImageIndices;
    imageIndex = 0;

    while (imageMask) {
        if (imageMask & 0x1) {
            int symbolCount = 0;

            for (int i = 0; i < renderer->currentSymbol; ++i) {
                struct SymbolLocation* target = &renderer->symbols[i];

                if (target->imageIndex != imageIndex) {
                    continue;
                }

                ++symbolCount;
            }

            if (symbolCount) {
                // 3 gfx per symbol, + 2 for color change + 1 for end display list
                prerender->displayLists[imageIndex] = malloc(sizeof(Gfx) * (symbolCount * 3 + 3));
            } else {
                prerender->displayLists[imageIndex] = NULL;
            }
        } else {
            prerender->displayLists[imageIndex] = NULL;
        }

        imageMask >>= 1;
        ++imageIndex;
    }
}

struct PrerenderedText* prerenderedTextNew(struct FontRenderer* renderer) {
    struct PrerenderedText* result = malloc(sizeof(struct PrerenderedText*));
    fontRendererInitPrerender(renderer, result);
    return result;
}

struct PrerenderedText* prerenderedTextCopy(struct PrerenderedText* text) {
    struct PrerenderedText* result = malloc(sizeof(struct PrerenderedText));

    int imageIndex = 0;
    int imageMask = text->usedImageIndices;

    while (imageMask) {
        imageMask >>= 1;
        ++imageIndex;
    }

    result->displayLists = malloc(sizeof(Gfx*) * imageIndex);
    result->usedImageIndices = text->usedImageIndices;
    result->x = text->x;
    result->y = text->y;
    result->width = text->width;
    result->height = text->height;

    imageIndex = 0;
    imageMask = text->usedImageIndices;

    while (imageMask) {
        if (imageMask & 0x1) {
            Gfx* src = text->displayLists[imageIndex];

            src += 2;
            while (_SHIFTR(src->words.w0, 24, 8) != G_ENDDL) {
                // copy image
                src += 3;
            }
            ++src;

            int size = (src - text->displayLists[imageIndex]) * sizeof(Gfx);

            result->displayLists[imageIndex] = malloc(size);

            Gfx* dest = result->displayLists[imageIndex];
            src = text->displayLists[imageIndex];
            // copy color 
            *dest++ = *src++;
            *dest++ = *src++;

            while (_SHIFTR(src->words.w0, 24, 8) != G_ENDDL) {
                // copy image
                *dest++ = *src++;
                *dest++ = *src++;
                *dest++ = *src++;
            }

            // copy end
            *dest++ = *src++;

            osWritebackDCache(result->displayLists[imageIndex], size);
        } else {
            result->displayLists[imageIndex] = NULL;
        }

        imageMask >>= 1;
        ++imageIndex;
    }
    return result;
}

void prerenderedTextCleanup(struct PrerenderedText* prerender) {
    int imageIndex = 0;
    int imageMask = prerender->usedImageIndices;

    while (imageMask) {
        free(prerender->displayLists[imageIndex]);

        imageMask >>= 1;
        ++imageIndex;
    }

    free(prerender->displayLists);
}


void prerenderedTextFree(struct PrerenderedText* prerender) {
    if (!prerender) {
        return;
    }

    prerenderedTextCleanup(prerender);
    free(prerender);
}

void prerenderShiftSingleSymbol(Gfx* gfx, int xOffset, int yOffset) {
    int x = _SHIFTR(gfx->words.w0, 12, 12) + xOffset;
    int y = _SHIFTL(gfx->words.w0, 0, 12) + yOffset;

    gfx->words.w0 = _SHIFTL(G_TEXRECT, 24, 8) | _SHIFTL(x, 12, 12) | _SHIFTL(y, 0, 12);

    x = _SHIFTR(gfx->words.w1, 12, 12) + xOffset;
    y = _SHIFTL(gfx->words.w1, 0, 12) + yOffset;

    gfx->words.w1 = _SHIFTL(G_TX_RENDERTILE, 24, 3) | _SHIFTL(x, 12, 12) | _SHIFTL(y, 0, 12);
}

void prerenderedTextRelocate(struct PrerenderedText* prerender, int x, int y) {
    int imageIndex = 0;
    int imageMask = prerender->usedImageIndices;

    int xOffset = (x - prerender->x) << 2;
    int yOffset = (y - prerender->y) << 2;

    while (imageMask) {
        if (imageMask & 0x1) {
            Gfx* gfx = prerender->displayLists[imageIndex];
            // skip color
            gfx += 2;

            while (_SHIFTR(gfx->words.w0, 24, 8) != G_ENDDL) {
                prerenderShiftSingleSymbol(gfx, xOffset, yOffset);
                gfx += 3;
            }

            osWritebackDCache(prerender->displayLists[imageIndex], (int)gfx - (int)prerender->displayLists[imageIndex]);
        }

        imageMask >>= 1;
        ++imageIndex;
    }
    prerender->x = x;
    prerender->y = y;
}

void prerenderedTextRecolor(struct PrerenderedText* prerender, struct Coloru8* color) {
    int imageIndex = 0;
    int imageMask = prerender->usedImageIndices;

    while (imageMask) {
        if (imageMask & 0x1) {
            Gfx* gfx = prerender->displayLists[imageIndex];

            if (color) {
                gDPPipeSync(gfx++);
                gDPSetEnvColor(gfx++, color->r, color->g, color->b, color->a);
            } else {
                gDPNoOp(gfx++);
                gDPNoOp(gfx++);
            }

            osWritebackDCache(prerender->displayLists[imageIndex], sizeof(Gfx) * 2);
        }

        imageMask >>= 1;
        ++imageIndex;
    }
}

void fontRendererFillPrerender(struct FontRenderer* renderer, struct PrerenderedText* prerender, int x, int y, struct Coloru8* color) {
    int imageIndex = 0;
    int imageMask = renderer->usedImageIndices & prerender->usedImageIndices;

    prerender->x = x;
    prerender->y = y;
    prerender->width = renderer->width;
    prerender->height = renderer->height;

    while (imageMask) {
        if (imageMask & 0x1) {
            Gfx* gfx = prerender->displayLists[imageIndex];

            if (color) {
                gDPPipeSync(gfx++);
                gDPSetEnvColor(gfx++, color->r, color->g, color->b, color->a);
            } else {
                gDPNoOp(gfx++);
                gDPNoOp(gfx++);
            }
            gfx = fontRendererBuildSingleGfx(renderer, imageIndex, x, y, gfx);
            gSPEndDisplayList(gfx++);

            osWritebackDCache(prerender->displayLists[imageIndex], (int)gfx - (int)prerender->displayLists[imageIndex]);
        }

        imageMask >>= 1;
        ++imageIndex;
    }
}

struct PrerenderedTextBatch* prerenderedBatchStart() {
    struct PrerenderedTextBatch* result = stackMalloc(sizeof(struct PrerenderedTextBatch));
    result->textCount = 0;
    result->usedImageIndices = 0;
    return result;
}

void prerenderedBatchAdd(struct PrerenderedTextBatch* batch, struct PrerenderedText* text, struct Coloru8* color) {
    if (batch->textCount >= MAX_PRERENDERED_STRINGS) {
        return;
    }
    prerenderedTextRecolor(text, color);
    batch->text[batch->textCount] = text;
    ++batch->textCount;
    batch->usedImageIndices |= text->usedImageIndices;
}

Gfx* prerenderedBatchFinish(struct PrerenderedTextBatch* batch, Gfx** fontImages, Gfx* gfx) {
    int imageIndex = 0;
    int imageMask = batch->usedImageIndices;
    int maskCheck = 1;

    while (imageMask) {
        if (imageMask & 0x1) {
            gSPDisplayList(gfx++, fontImages[imageIndex]);

            for (int i = 0; i < batch->textCount; ++i) {
                if (batch->text[i]->usedImageIndices & maskCheck) {
                    gSPDisplayList(gfx++, batch->text[i]->displayLists[imageIndex]);
                }
            }
        }

        imageMask >>= 1;
        maskCheck <<= 1;
        ++imageIndex;
    }

    stackMallocFree(batch);

    return gfx;
}