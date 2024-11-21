#include "locales.h"

#include "savefile/savefile.h"

#include "codegen/src/audio/languages.h"

int mapLocaleSound(int soundId) {
    // Is this a localized sound?
    if (soundId >= FIRST_LOCALIZED_SOUND &&
        soundId < (FIRST_LOCALIZED_SOUND + NUM_LOCALIZED_SOUNDS)) {

        int language = (int)gSaveData.audio.audioLanguage;
        if (language > 0) {
            // Non-default language
            soundId = AudioLanguageValues[language - 1][soundId - FIRST_LOCALIZED_SOUND];
        }
    }

    return soundId;
}
