#ifndef _SOUND_PLAYER_H
#define _SOUND_PLAYER_H

#include <ultra64.h>
#include "math/vector3.h"
#include "math/quaternion.h"

#define MAX_SOUNDS 128

#define SOUND_SAMPLE_RATE 22500

#define SOUND_ID_NONE -1

extern char _soundsSegmentRomStart[];
extern char _soundsSegmentRomEnd[];
extern char _soundsTblSegmentRomStart[];
extern char _soundsTblSegmentRomEnd[];

void soundPlayerInit();
void soundPlayerUpdate();
ALSndId soundPlayerPlay(int soundClipId, float volume, float pitch);
int soundPlayerIsPlaying(ALSndId soundId);

#endif