#ifndef __AUDIO_H__
#define __AUDIO_H__

#include <stdint.h>

#define AUDIO_OUTPUT_HZ 22050
#define SOUND_ID_NONE   -1

typedef int16_t SoundId;

void* audioInit(void* memoryEnd, int maxVoices);
void audioUpdate();

SoundId audioPlaySound(int soundClipId, float volume, float pitch, float pan, float echo);
void audioSetSoundParams(SoundId soundId, float volume, float pitch, float pan, float echo);
int audioIsSoundPlaying(SoundId soundId);
void audioPauseSound(SoundId soundId);
void audioResumeSound(SoundId soundId);
void audioStopSound(SoundId soundId);
void audioFreeSound(SoundId soundId);

int audioIsSoundClipLooped(int soundClipId);

#endif
