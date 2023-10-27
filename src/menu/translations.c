#include "translations.h"

#include "../util/memory.h"
#include "../util/rom.h"

#include "../build/src/audio/subtitles.h"

char** gCurrentTranslations = NULL;

void translationsLoad(int language) {
    if (NUM_SUBTITLE_LANGUAGES == 0) {
        gCurrentTranslations = NULL;
        return;
    }

    if (language < 0) {
        language = 0;
    }

    if (language >= NUM_SUBTITLE_LANGUAGES) {
        language = NUM_SUBTITLE_LANGUAGES - 1;
    }

    struct SubtitleBlock* block = &SubtitleLanguageBlocks[language];

    int blockSize = (int)block->romEnd - (int)block->romStart;
    char* blockStart = malloc(blockSize);
    romCopy(block->romStart, blockStart, blockSize);

    gCurrentTranslations = CALC_RAM_POINTER(block->values, blockStart);

    for (int i = 0; i < NUM_SUBTITLE_MESSAGES; ++i) {
        gCurrentTranslations[i] = CALC_RAM_POINTER(gCurrentTranslations[i], blockStart);
    }
}

void translationsReload(int language) {
    free(gCurrentTranslations);
    translationsLoad(language);
}

char* translationsGet(int message) {
    if (message < 0 || message >= NUM_SUBTITLE_MESSAGES || !gCurrentTranslations) {
        return "";
    }

    return gCurrentTranslations[message];
}