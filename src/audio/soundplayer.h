#ifndef _SOUND_PLAYER_H
#define _SOUND_PLAYER_H

#include <ultra64.h>
#include "math/vector3.h"
#include "math/quaternion.h"

#define MAX_SOUNDS 128

#define MAX_SOUND_LISTENERS 3

#define SOUND_SAMPLE_RATE 22500

#define SOUND_ID_NONE -1

extern char _soundsSegmentRomStart[];
extern char _soundsSegmentRomEnd[];
extern char _soundsTblSegmentRomStart[];
extern char _soundsTblSegmentRomEnd[];

void soundPlayerInit();
void soundPlayerUpdate();
ALSndId soundPlayerPlay(int soundClipId, float volume, float pitch, struct Vector3* at, struct Vector3* velocity);
float soundClipDuration(int soundClipId, float pitch);
void soundPlayerStop(ALSndId soundId);
void soundPlayerStopAll();

void soundPlayerPause();
void soundPlayerResume();

void soundPlayerUpdatePosition(ALSndId soundId, struct Vector3* at, struct Vector3* velocity);
void soundPlayerAdjustVolume(ALSndId soundId, float newVolume);

int soundPlayerIsPlaying(ALSndId soundId);
float soundPlayerTimeLeft(ALSndId soundId);

void soundListenerUpdate(struct Vector3* position, struct Quaternion* rotation, struct Vector3* velocity, int listenerIndex);
void soundListenerSetCount(int count);

#endif