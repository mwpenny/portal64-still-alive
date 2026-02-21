#ifndef _SOUND_PLAYER_H
#define _SOUND_PLAYER_H

#include <ultra64.h>
#include "math/vector3.h"
#include "math/quaternion.h"

#define MAX_SOUND_LISTENERS 3

#define SOUND_ID_NONE -1

enum SoundType {
    SoundTypeNone,
    SoundTypeMusic,
    SoundTypeVoice,
    SoundTypeAmbience,
    SoundTypeAll,
};

typedef ALSndId SoundId;

void* soundPlayerInit(void* memoryEnd);
void soundPlayerGameVolumeUpdate();
void soundPlayerUpdate();
ALSndId soundPlayerPlay(int soundClipId, float volume, float pitch, struct Vector3* at, struct Vector3* velocity, enum SoundType type);
void soundPlayerStop(ALSndId soundId);
void soundPlayerStopAll();

void soundPlayerPause();
void soundPlayerResume();

void soundPlayerUpdatePosition(ALSndId soundId, struct Vector3* at, struct Vector3* velocity);
float soundPlayerGetOriginalVolume(ALSndId soundId);
void soundPlayerAdjustVolume(ALSndId soundId, float newVolume);
void soundPlayerFadeOutsideRadius(float volumePercent, struct Vector3* origin, float radius, int persistent);

int soundPlayerIsPlaying(ALSndId soundId);
int soundPlayerIsLoopedById(int soundId);

void soundListenerUpdate(struct Vector3* position, struct Vector3* right, struct Vector3* velocity, int listenerIndex);
void soundListenerSetCount(int count);

#endif