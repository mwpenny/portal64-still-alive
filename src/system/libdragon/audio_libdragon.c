#include "system/audio.h"

// TODO

void* audioInit(void* memoryEnd, int maxVoices) {
}

void audioUpdate() {
}

VoiceId audioPlaySound(int soundClipId, float volume, float pitch, float pan, float echo) {
    return VOICE_ID_NONE;
}

void audioSetSoundParams(VoiceId voiceId, float volume, float pitch, float pan, float echo) {
}

int audioIsSoundPlaying(VoiceId voiceId) {
    return 0;
}

int audioIsSoundLooped(VoiceId voiceId) {
    return 0;
}

void audioPauseSound(VoiceId voiceId) {
}

int audioIsSoundPaused(VoiceId voiceId) {
    return 0;
}

void audioResumeSound(VoiceId voiceId) {
}

void audioStopSound(VoiceId voiceId) {
}

void audioReleaseVoice(VoiceId voiceId) {
}
