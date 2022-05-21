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
    u16 newSoundTicks;
    float estimatedTimeLeft;
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

float soundPlayerEstimateLength(ALSound* sound, float speed) {
    if (!sound->wavetable) {
        return 0.0f;
    }

    int sampleCount = 0;

    if (sound->wavetable->type == AL_ADPCM_WAVE) {
        sampleCount = sound->wavetable->len * 16 / 9;
    } else {
        sampleCount = sound->wavetable->len >> 1;
    }

    return sampleCount * (1.0f / OUTPUT_RATE) / speed;
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
    gActiveSounds[gActiveSoundCount].newSoundTicks = 10;
    gActiveSounds[gActiveSoundCount].estimatedTimeLeft = soundPlayerEstimateLength(gSoundClipArray->sounds[soundClipId], pitch);

    alSndpSetSound(&gSoundPlayer, result);
    alSndpSetVol(&gSoundPlayer, (short)(32767 * volume));
    alSndpSetPitch(&gSoundPlayer, pitch);
    alSndpPlay(&gSoundPlayer);

    ++gActiveSoundCount;

    return result;
}

void soundPlayerUpdate() {
    int index = 0;
    int writeIndex = 0;

    while (index < gActiveSoundCount) {
        struct ActiveSound* sound = &gActiveSounds[index];

        if (sound->newSoundTicks) {
            --sound->newSoundTicks;
        }

        sound->estimatedTimeLeft -= FIXED_DELTA_TIME;

        alSndpSetSound(&gSoundPlayer, sound->soundId);

        if (alSndpGetState(&gSoundPlayer) == AL_STOPPED && !sound->newSoundTicks) {
            alSndpDeallocate(&gSoundPlayer, sound->soundId);
            sound->soundId = SOUND_ID_NONE;
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

struct ActiveSound* soundPlayerFindActiveSound(ALSndId soundId) {
    if (soundId == SOUND_ID_NONE) {
        return NULL;
    }
    
    for (int i = 0; i < gActiveSoundCount; ++i) {
        if (gActiveSounds[i].soundId == soundId) {
            return &gActiveSounds[i];
        }
    }

    return NULL;
}

int soundPlayerIsPlaying(ALSndId soundId) {
    struct ActiveSound* activeSound = soundPlayerFindActiveSound(soundId);

    if (!activeSound) {
        return 0;
    }

    if (activeSound->newSoundTicks) {
        return 1;
    }

    alSndpSetSound(&gSoundPlayer, soundId);
    return activeSound->estimatedTimeLeft > 0.0f && alSndpGetState(&gSoundPlayer) != AL_STOPPED;
}