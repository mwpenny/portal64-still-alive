#include "audio.h"
#include "soundplayer.h"
#include "soundarray.h"
#include "util/rom.h"
#include "util/time.h"
#include "math/mathf.h"

struct SoundArray* gSoundClipArray;
ALSndPlayer gSoundPlayer;

#define MAX_ACTIVE_SOUNDS   16

#define SOUND_FLAGS_3D          (1 << 0)
#define SOUND_FLAGS_LOOPING     (1 << 1)

struct ActiveSound {
    ALSndId soundId;
    u16 flags;
    u16 newSoundTicks;
    float estimatedTimeLeft;
    struct Vector3 pos3D;
    float volume;
};

struct SoundListener {
    struct Vector3 worldPos;
    struct Vector3 rightVector;
};

struct ActiveSound gActiveSounds[MAX_ACTIVE_SOUNDS];
int gActiveSoundCount = 0;

struct SoundListener gSoundListeners[MAX_SOUND_LISTENERS];
int gActiveListenerCount = 0;

void soundPlayerDetermine3DSound(struct Vector3* at, float* volumeIn, float* volumeOut, int* panOut) {
    if (!gActiveListenerCount) {
        *volumeOut = *volumeIn;
        *panOut = 64;
        return;
    }

    struct SoundListener* nearestListener = &gSoundListeners[0];
    float distance = vector3DistSqrd(at, &gSoundListeners[0].worldPos);

    for (int i = 1; i < MAX_SOUND_LISTENERS; ++i) {
        float check = vector3DistSqrd(at, &gSoundListeners[i].worldPos);

        if (check < distance) {
            distance = check;
            nearestListener = &gSoundListeners[i];
        }
    }

    if (distance < 0.0000001f) {
        *volumeOut = *volumeIn;
        *panOut = 64;
        return;
    }

    // attenuate the volume
    *volumeOut = *volumeIn / distance;

    // clamp to full volume
    if (*volumeOut > 1.0f) {
        *volumeOut = 1.0f;
    }

    struct Vector3 offset;
    vector3Sub(at, &nearestListener->worldPos, &offset);

    float pan = -vector3Dot(&offset, &nearestListener->rightVector) / sqrtf(distance);

    pan = pan * 64.0f + 64.0f;

    *panOut = (int)pan;

    if (*panOut < 0) {
        *panOut = 0;
    } 

    if (*panOut > 127) {
        *panOut = 127;
    }
}

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

int soundPlayerIsLooped(ALSound* sound) {
    if (!sound->wavetable) {
        return 0;
    }

    if (sound->wavetable->type == AL_ADPCM_WAVE) {
        return sound->wavetable->waveInfo.adpcmWave.loop != NULL;
    } else {
        return sound->wavetable->waveInfo.rawWave.loop != NULL;
    }
}

float soundPlayerEstimateLength(ALSound* sound, float speed) {
    if (!sound->wavetable) {
        return 0.0f;
    }

    if (soundPlayerIsLooped(sound)) {
        return 1000000000000000000000.0f;
    }

    int sampleCount = 0;

    if (sound->wavetable->type == AL_ADPCM_WAVE) {
        sampleCount = sound->wavetable->len * 16 / 9;
    } else {
        sampleCount = sound->wavetable->len >> 1;
    }

    return sampleCount * (1.0f / OUTPUT_RATE) / speed;
}

ALSndId soundPlayerPlay(int soundClipId, float volume, float pitch, struct Vector3* at) {
    if (gActiveSoundCount == MAX_ACTIVE_SOUNDS || soundClipId < 0 || soundClipId >= gSoundClipArray->soundCount) {
        return SOUND_ID_NONE;
    }
    
    ALSound* alSound = gSoundClipArray->sounds[soundClipId];

    ALSndId result = alSndpAllocate(&gSoundPlayer, alSound);

    if (result == SOUND_ID_NONE) {
        return result;
    }

    struct ActiveSound* sound = &gActiveSounds[gActiveSoundCount];

    sound->soundId = result;
    sound->newSoundTicks = 10;
    sound->flags = 0;
    sound->estimatedTimeLeft = soundPlayerEstimateLength(alSound, pitch);
    sound->volume = volume;

    float startingVolume = volume;
    int panning = 64;

    if (at) {
        sound->flags |= SOUND_FLAGS_3D;
        sound->pos3D = *at;
        soundPlayerDetermine3DSound(at, &startingVolume, &startingVolume, &panning);
    }

    if (soundPlayerIsLooped(alSound)) {
        sound->flags |= SOUND_FLAGS_LOOPING;
    }

    alSndpSetSound(&gSoundPlayer, result);
    alSndpSetVol(&gSoundPlayer, (short)(32767 * startingVolume));
    alSndpSetPitch(&gSoundPlayer, pitch);
    alSndpSetPan(&gSoundPlayer, panning);
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
            if (sound->flags & SOUND_FLAGS_3D) {
                float volume;
                int panning;

                soundPlayerDetermine3DSound(&sound->pos3D, &sound->volume, &volume, &panning);
                alSndpSetVol(&gSoundPlayer, (short)(32767 * volume));
                alSndpSetPan(&gSoundPlayer, panning);
            }

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


void soundPlayerStop(ALSndId soundId) {
    struct ActiveSound* activeSound = soundPlayerFindActiveSound(soundId);

    if (activeSound) {
        alSndpSetSound(&gSoundPlayer, soundId);
        alSndpStop(&gSoundPlayer);
        activeSound->estimatedTimeLeft = 0.0f;
    }
}

void soundPlayerUpdatePosition(ALSndId soundId, struct Vector3* at) {
    struct ActiveSound* activeSound = soundPlayerFindActiveSound(soundId);

    if (activeSound) {
        activeSound->flags |= SOUND_FLAGS_3D;
        activeSound->pos3D = *at;
    }
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

void soundListenerUpdate(struct Vector3* position, struct Quaternion* rotation, int listenerIndex) {
    gSoundListeners[listenerIndex].worldPos = *position;
    quatMultVector(rotation, &gRight, &gSoundListeners[listenerIndex].rightVector);
}

void soundListenerSetCount(int count) {
    gActiveListenerCount = count;
}