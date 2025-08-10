#ifndef _SOUND_PLAYER_H
#define _SOUND_PLAYER_H

#include <ultra64.h>
#include "math/vector3.h"
#include "math/quaternion.h"

#define MAX_SOUNDS 32

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

void soundPlayerInit();
void soundPlayerGameVolumeUpdate();
void soundPlayerUpdate();
ALSndId soundPlayerPlay(int soundClipId, float volume, float pitch, struct Vector3* at, struct Vector3* velocity, enum SoundType type);
float soundClipDuration(int soundClipId, float pitch);
void soundPlayerStop(ALSndId soundId);
void soundPlayerStopAll();

void soundPlayerPause();
void soundPlayerResume();

void soundPlayerUpdatePosition(ALSndId soundId, struct Vector3* at, struct Vector3* velocity);
void soundPlayerAdjustVolume(ALSndId soundId, float newVolume);

int soundPlayerIsPlaying(ALSndId soundId);
int soundPlayerIsLoopedById(int soundId);
float soundPlayerTimeLeft(ALSndId soundId);

void soundListenerUpdate(struct Vector3* position, struct Quaternion* rotation, struct Vector3* velocity, int listenerIndex);
void soundListenerSetCount(int count);

#endif