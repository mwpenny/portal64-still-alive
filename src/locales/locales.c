#include "locales.h"
#include "../build/src/audio/languages.h"
#include "../savefile/savefile.h"

int getAudioLanguage() {
  return (int)gSaveData.audio.audioLanguage;
}

int mapLocaleSound(int soundId) {
    int language = getAudioLanguage();
    
    switch(language) {
        default:
            soundId = AudioLanguageValues[language][soundId];
            break;
        case AUDIO_LANGUAGE_EN:       
            break;
    }
          
    return soundId;
}
