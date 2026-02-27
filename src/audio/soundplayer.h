#ifndef __SOUND_PLAYER_H__
#define __SOUND_PLAYER_H__

#include "math/vector3.h"
#include "system/audio.h"

#define MAX_SKIPPABLE_SOUNDS    18
#define MAX_ACTIVE_SOUNDS       24

enum SoundType {
    SoundTypeNone,
    SoundTypeMusic,
    SoundTypeVoice,
    SoundTypeAmbience,
    SoundTypeAll,
};

void* soundPlayerInit(void* memoryEnd);
void soundPlayerUpdate();
int soundPlayerSoundCount();

SoundId soundPlayerPlay(int soundClipId, float volume, float pitch, struct Vector3* position, struct Vector3* velocity, enum SoundType type);
int soundPlayerIsPlaying(SoundId soundId);
int soundPlayerIsLooped(SoundId soundId);
int soundPlayerIsMuted(SoundId soundId);
void soundPlayerSetPosition(SoundId soundId, struct Vector3* position, struct Vector3* velocity);
void soundPlayerSetVolume(SoundId soundId, float newVolume);
void soundPlayerStop(SoundId soundId);

void soundPlayerStopAll();
void soundPlayerPause();
void soundPlayerResume();
void soundPlayerFadeOutsideRadius(float volumePercent, struct Vector3* origin, float radius, int persistent);
void soundPlayerUpdateVolumeLevels();

void soundListenerUpdate(int listenerIndex, struct Vector3* position, struct Vector3* right, struct Vector3* velocity);
void soundListenerSetCount(int count);

#endif
