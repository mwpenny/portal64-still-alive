#ifndef __AUDIO_H__
#define __AUDIO_H__

#include <stdint.h>

#define AUDIO_OUTPUT_HZ 22050
#define VOICE_ID_NONE   -1

typedef int16_t VoiceId;

void* audioInit(void* memoryEnd, int maxVoices);
void audioUpdate();

VoiceId audioPlaySound(int soundClipId, float volume, float pitch, float pan, float echo);
void audioSetSoundParams(VoiceId voiceId, float volume, float pitch, float pan, float echo);
int audioIsSoundPlaying(VoiceId voiceId);
int audioIsSoundLooped(VoiceId voiceId);
void audioPauseSound(VoiceId voiceId);
int audioIsSoundPaused(VoiceId voiceId);
void audioResumeSound(VoiceId voiceId);
void audioStopSound(VoiceId voiceId);
void audioReleaseVoice(VoiceId voiceId);

#endif
