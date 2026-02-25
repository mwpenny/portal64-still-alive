#include "system/audio.h"

// TODO

void* audioInit(void* memoryEnd, int maxVoices) {
}

void audioUpdate() {
}

SoundId audioPlaySound(int soundClipId, float volume, float pitch, float pan, float echo) {
    return SOUND_ID_NONE;
}

void audioSetSoundParams(SoundId soundId, float volume, float pitch, float pan, float echo) {
}

int audioIsSoundPlaying(SoundId soundId) {
    return 0;
}

void audioPauseSound(SoundId soundId) {
}

void audioResumeSound(SoundId soundId) {
}

void audioStopSound(SoundId soundId) {
}

void audioFreeSound(SoundId soundId) {
}

int audioIsSoundClipLooped(int soundClipId) {
    return 0;
}
