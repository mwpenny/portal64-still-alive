#ifndef _SOUND_PLAYER_H
#define _SOUND_PLAYER_H

#include "math/vector3.h"
#include "math/quaternion.h"
#include "system/audio.h"

#define MAX_SOUND_LISTENERS 3

#define SOUND_ID_NONE -1

enum SoundType {
    SoundTypeNone,
    SoundTypeMusic,
    SoundTypeVoice,
    SoundTypeAmbience,
    SoundTypeAll,
};

void* soundPlayerInit(void* memoryEnd);
void soundPlayerGameVolumeUpdate();
void soundPlayerUpdate();
SoundId soundPlayerPlay(int soundClipId, float volume, float pitch, struct Vector3* at, struct Vector3* velocity, enum SoundType type);
void soundPlayerStop(SoundId soundId);
void soundPlayerStopAll();

void soundPlayerPause();
void soundPlayerResume();

void soundPlayerUpdatePosition(SoundId soundId, struct Vector3* at, struct Vector3* velocity);
float soundPlayerGetOriginalVolume(SoundId soundId);
void soundPlayerAdjustVolume(SoundId soundId, float newVolume);
void soundPlayerFadeOutsideRadius(float volumePercent, struct Vector3* origin, float radius, int persistent);

int soundPlayerIsPlaying(SoundId soundId);
int soundPlayerIsLooped(SoundId soundId);

void soundListenerUpdate(struct Vector3* position, struct Vector3* right, struct Vector3* velocity, int listenerIndex);
void soundListenerSetCount(int count);

#endif