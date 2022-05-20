#include "audio.h"
#include "soundplayer.h"
#include "soundarray.h"
#include "util/rom.h"
#include "util/time.h"
#include "math/mathf.h"

struct SoundArray* gSoundClipArray;
ALSndPlayer gSoundPlayer;

#define MAX_ACTIVE_SOUNDS   16

struct ActiveSound {
    ALSndId soundId;
    u16 soundId3d;
};

struct ActiveSound gActiveSounds[MAX_ACTIVE_SOUNDS];
int gActiveSoundCount = 0;

void soundPlayerInit() {
    gSoundClipArray = alHeapAlloc(&gAudioHeap, 1, _soundsSegmentRomEnd - _soundsSegmentRomStart);
    romCopy(_soundsSegmentRomStart, (char*)gSoundClipArray, _soundsSegmentRomEnd - _soundsSegmentRomStart);
    soundArrayInit(gSoundClipArray, _soundsTblSegmentRomStart);

    ALSndpConfig sndConfig;
    sndConfig.maxEvents = MAX_EVENTS;
    sndConfig.maxSounds = MAX_SOUNDS;
    sndConfig.heap = &gAudioHeap;
    alSndpNew(&gSoundPlayer, &sndConfig);

    for (int i = 0; i < MAX_ACTIVE_SOUNDS; ++i) {
        gActiveSounds[i].soundId = SOUND_ID_NONE;
    }
}

ALSndId soundPlayerPlay(int soundClipId, float volume, float pitch) {
    if (gActiveSoundCount == MAX_ACTIVE_SOUNDS || soundClipId < 0 || soundClipId >= gSoundClipArray->soundCount) {
        return SOUND_ID_NONE;
    }

    ALSndId result = alSndpAllocate(&gSoundPlayer, gSoundClipArray->sounds[soundClipId]);

    if (result == SOUND_ID_NONE) {
        return result;
    }

    gActiveSounds[gActiveSoundCount].soundId = result;
    gActiveSounds[gActiveSoundCount].soundId3d = SOUND_ID_NONE;

    alSndpSetSound(&gSoundPlayer, result);
    alSndpSetVol(&gSoundPlayer, (short)(32767 * volume));
    alSndpSetPitch(&gSoundPlayer, pitch);
    alSndpPlay(&gSoundPlayer);

    return result;
}

void soundPlayerUpdate() {
    int index = 0;
    int writeIndex = 0;

    while (index < gActiveSoundCount) {
        alSndpSetSound(&gSoundPlayer, gActiveSounds[index].soundId);

        if (alSndpGetState(&gSoundPlayer) == AL_STOPPED) {
            alSndpDeallocate(&gSoundPlayer, gActiveSounds[index].soundId);
        } else {
            ++writeIndex;
        }
        
        ++index;

        if (writeIndex != index) {
            gActiveSounds[writeIndex] = gActiveSounds[index];
        }
    }

    gActiveSoundCount = writeIndex;
}