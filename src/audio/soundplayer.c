#include "soundplayer.h"

#include "audio.h"
#include "clips.h"
#include "math/mathf.h"
#include "math/transform.h"
#include "physics/collision_scene.h"
#include "savefile/savefile.h"
#include "soundarray.h"
#include "system/cartridge.h"
#include "system/time.h"
#include "util/frame_time.h"

extern char _soundsSegmentRomStart[];
extern char _soundsSegmentRomEnd[];
extern char _soundsTblSegmentRomStart[];
extern char _soundsTblSegmentRomEnd[];

struct SoundArray* gSoundClipArray;
ALSndPlayer gSoundPlayer;

#define MAX_SKIPPABLE_SOUNDS    6
#define MAX_ACTIVE_SOUNDS       12

#define SOUND_FLAGS_3D          (1 << 0)
#define SOUND_FLAGS_LOOPING     (1 << 1)
#define SOUND_HAS_STARTED       (1 << 2)
#define SOUND_FLAGS_PAUSED      (1 << 3)

#define SPEED_OF_SOUND          343.2f
#define VOLUME_FADE_THRESHOLD   0.055f
#define VOLUME_FADE_DISTANCE    3.5f
#define VOLUME_CURVE_PAD        0.0125f
#define VOLUME_AMPLIFICATION    1.538f
#define VOICE_FX_MIX            32
#define FX_PEAK_DISTANCE        25.0f
#define FX_MIN_AMOUNT           0.1f
#define FX_MAX_AMOUNT           0.6f
#define DEFAULT_FX_MIX          8 // Small amount of FX mix for all sounds.

struct ActiveSound {
    ALSndId soundId;
    u16 flags;
    Time timeRemaining;
    struct Vector3 pos3D;
    struct Vector3 velocity3D;
    float volume;
    float originalVolume;
    float basePitch;
    enum SoundType soundType;
};

struct SoundListener {
    struct Vector3 worldPos;
    struct Vector3 rightVector;
    struct Vector3 velocity;
};

struct ActiveSound gActiveSounds[MAX_ACTIVE_SOUNDS];
int gActiveSoundCount = 0;

struct SoundListener gSoundListeners[MAX_SOUND_LISTENERS];
int gActiveListenerCount = 0;

void soundPlayerDetermine3DSound(struct Vector3* at, struct Vector3* velocity, float* volumeIn, float* volumeOut, int* panOut, float* pitchBend, int* fxMix) {
    *panOut = 64;
    *pitchBend = 1.0f;
    *fxMix = 0;

    if (!gActiveListenerCount) {
        *volumeOut = *volumeIn;
        return;
    }

    struct SoundListener* nearestListener = &gSoundListeners[0];
    float distance = vector3DistSqrd(at, &gSoundListeners[0].worldPos);

    for (int i = 1; i < gActiveListenerCount; ++i) {
        float check = vector3DistSqrd(at, &gSoundListeners[i].worldPos);

        if (check < distance) {
            distance = check;
            nearestListener = &gSoundListeners[i];
        }
    }

    if (distance < 0.0000001f) {
        *volumeOut = *volumeIn;
        return;
    }

    float distanceSqrt = sqrtf(distance);

    // Initial linear volume level
    float volumeLevel = clampf(*volumeIn / distanceSqrt, 0.0f, 1.0f);

    // Fade quiet sounds more aggressively
    if (volumeLevel > 0.0f && volumeLevel < VOLUME_FADE_THRESHOLD) {
        // How far is the listener from where the threshold volume was reached?
        float fadeStartDist = *volumeIn * (1.0f / VOLUME_FADE_THRESHOLD);
        float distFromFadeStart = distanceSqrt - fadeStartDist;

        float fadeAmount = clampf(distFromFadeStart * (1.0f / VOLUME_FADE_DISTANCE), 0.0f, 1.0f);
        volumeLevel *= (1.0f - fadeAmount);
    }

    // Fudge with the volume curve a bit. 
    // Try to make distant sounds more apparent while
    // compressing the volume of closer sounds.
    volumeLevel = clampf((volumeLevel - VOLUME_CURVE_PAD) * VOLUME_AMPLIFICATION, 0.0f, 1.0f);

    if (volumeLevel == 0.0f) {
        *volumeOut = 0.0f;
        return;
    }

    *volumeOut = volumeLevel;

    // Add FX/reverb amount.
    float fxDistCurve = clampf(
        mathfLerp(FX_MIN_AMOUNT, FX_MAX_AMOUNT, distanceSqrt / FX_PEAK_DISTANCE),
        FX_MIN_AMOUNT, FX_MAX_AMOUNT
    );
    *fxMix = (int)(127.0f * fxDistCurve);

    struct Vector3 offset;
    vector3Sub(at, &nearestListener->worldPos, &offset);

    struct Vector3 relativeVelocity;
    vector3Sub(velocity, &nearestListener->velocity, &relativeVelocity);

    float invDist = 1.0f / distanceSqrt;

    float directionalVelocity = -vector3Dot(&offset, &relativeVelocity) * invDist;

    *pitchBend = (SPEED_OF_SOUND + directionalVelocity) * (1.0f / SPEED_OF_SOUND);

    float pan = vector3Dot(&offset, &nearestListener->rightVector) * invDist;

    pan = pan * 64.0f + 64.0f;

    *panOut = MAX(0, MIN((int)pan, 127));
}

static void soundInitDecayTimes(struct SoundArray* soundArray) {
    for (int i = 0; i < soundArray->soundCount; ++i) {
        ALSound* sound = soundArray->sounds[i];

        // When pausing the game we also want to pause all sounds. Libultra
        // doesn't provide a good way to do this, or to start sounds partway
        // through. So to "pause" sounds the player sets the pitch to 0.
        // Unfortunately mid-playback pitch changes don't affect the envelope
        // and so sounds will end at the same time regardless of the pause.
        //
        // This is a bit of a hack to get around the problem. All sounds are
        // given infinite duration and the sound player will stop them at
        // the proper time.
        sound->envelope->decayTime = -1;
    }
}

void soundPlayerInit() {
    gSoundClipArray = alHeapAlloc(&gAudioHeap, 1, _soundsSegmentRomEnd - _soundsSegmentRomStart);
    romCopy(_soundsSegmentRomStart, (char*)gSoundClipArray, _soundsSegmentRomEnd - _soundsSegmentRomStart);
    soundArrayInit(gSoundClipArray, _soundsTblSegmentRomStart);

    soundInitDecayTimes(gSoundClipArray);

    ALSndpConfig sndConfig;
    sndConfig.maxEvents = MAX_EVENTS;
    sndConfig.maxSounds = MAX_ACTIVE_SOUNDS;
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

Time soundPlayerCalculateLength(ALSound* sound, float speed) {
    if (!sound->wavetable) {
        return 0.0f;
    }

    if (soundPlayerIsLooped(sound)) {
        return -1;
    }

    int sampleCount = 0;

    if (sound->wavetable->type == AL_ADPCM_WAVE) {
        sampleCount = sound->wavetable->len * 16 / 9;
    } else {
        sampleCount = sound->wavetable->len >> 1;
    }

    return timeFromSeconds(sampleCount * (1.0f / OUTPUT_RATE) / speed);
}

ALSndId soundPlayerPlay(int soundClipId, float volume, float pitch, struct Vector3* at, struct Vector3* velocity, enum SoundType type) {
    if (gActiveSoundCount >= MAX_ACTIVE_SOUNDS || soundClipId < 0 || soundClipId >= gSoundClipArray->soundCount) {
        return SOUND_ID_NONE;
    }
    if (gActiveSoundCount >= MAX_SKIPPABLE_SOUNDS && clipsCheckSoundSkippable(soundClipId)) {
        return SOUND_ID_NONE;
    }

    ALSound* alSound = gSoundClipArray->sounds[soundClipId];

    ALSndId result = alSndpAllocate(&gSoundPlayer, alSound);

    if (result == SOUND_ID_NONE) {
        return result;
    }

    struct ActiveSound* sound = &gActiveSounds[gActiveSoundCount];

    sound->soundId = result;
    sound->flags = 0;
    sound->timeRemaining = soundPlayerCalculateLength(alSound, pitch);
    sound->volume = volume;
    sound->originalVolume = volume;
    sound->basePitch = pitch;
    sound->soundType = type;

    float newVolume = sound->originalVolume * gSaveData.audio.soundVolume/0xFFFF;
    if (type == SoundTypeMusic) {
        newVolume = newVolume * gSaveData.audio.musicVolume/0xFFFF;
    }
    sound->volume = newVolume;

    int panning = 64;
    int fxMix = DEFAULT_FX_MIX;

    if (at) {
        sound->flags |= SOUND_FLAGS_3D;
        sound->pos3D = *at;
        sound->velocity3D = *velocity;
        float pitchBend;
        soundPlayerDetermine3DSound(at, velocity, &newVolume, &newVolume, &panning, &pitchBend, &fxMix);
        pitch = pitch * pitchBend;
    }

    if (soundPlayerIsLooped(alSound)) {
        sound->flags |= SOUND_FLAGS_LOOPING;
    }

    alSndpSetSound(&gSoundPlayer, result);
    alSndpSetVol(&gSoundPlayer, (short)(32767 * newVolume));
    alSndpSetPitch(&gSoundPlayer, pitch);
    alSndpSetPan(&gSoundPlayer, panning);

    // Add reverb effect.
    if (type == SoundTypeAll) {
        alSndpSetFXMix(&gSoundPlayer, fxMix);
    } else if (type == SoundTypeVoice) {
        alSndpSetFXMix(&gSoundPlayer, VOICE_FX_MIX);
    }

    alSndpPlay(&gSoundPlayer);

    ++gActiveSoundCount;

    return result;
}

void soundPlayerGameVolumeUpdate() {
    for (int i = 0; i < gActiveSoundCount; ++i) {
        struct ActiveSound* sound = &gActiveSounds[i];
        if (!sound){
            continue;
        }

        float newVolume = (sound->originalVolume * gSaveData.audio.soundVolume) / 0xFFFF;
        if (sound->soundType == SoundTypeMusic) {
            newVolume = (newVolume * gSaveData.audio.musicVolume) / 0xFFFF;
        }
        
        if (sound->flags & SOUND_FLAGS_PAUSED) {
            sound->volume = newVolume;
        } else if (sound->flags & SOUND_FLAGS_3D) {
            sound->volume = newVolume;
            float volume;
            float pitch;
            int panning;
            int fxMix;
            soundPlayerDetermine3DSound(&sound->pos3D, &sound->velocity3D, &sound->volume, &volume, &panning, &pitch, &fxMix);
            alSndpSetSound(&gSoundPlayer, sound->soundId);
            alSndpSetVol(&gSoundPlayer, (short)(32767 * volume));
            alSndpSetPan(&gSoundPlayer, panning);
            alSndpSetPitch(&gSoundPlayer, sound->basePitch * pitch);
            alSndpSetFXMix(&gSoundPlayer, fxMix);
        } else {
            alSndpSetSound(&gSoundPlayer, sound->soundId);
            alSndpSetVol(&gSoundPlayer, (short)(32767 * newVolume));
            sound->volume = newVolume;
        }
    }
}

#define SOUND_DAMPING_LEVEL 0.5f

void soundPlayerUpdate() {
    static float soundDamping = 1.0f;
    static Time lastUpdate = 0;

    int index = 0;
    int writeIndex = 0;
    int isVoiceActive = 0;

    Time now = timeGetTime();
    Time timeDelta = now - lastUpdate;
    lastUpdate = now;

    while (index < gActiveSoundCount) {
        struct ActiveSound* sound = &gActiveSounds[index];

        if (sound->flags & SOUND_FLAGS_PAUSED) {
            ++writeIndex;
            ++index;
            continue;
        }

        alSndpSetSound(&gSoundPlayer, sound->soundId);

        if ((sound->flags & (SOUND_HAS_STARTED | SOUND_FLAGS_LOOPING)) == SOUND_HAS_STARTED &&
            sound->timeRemaining > 0
        ) {
            // Check if it is time for the sound to stop. We must manage this
            // ourselves due how sounds are paused. See soundInitDecayTimes().
            if (sound->timeRemaining > timeDelta) {
                sound->timeRemaining -= timeDelta;
            } else {
                sound->timeRemaining = 0;
                alSndpStop(&gSoundPlayer);
            }
        }

        if (sound->soundType == SoundTypeVoice) {
            isVoiceActive = 1;
        }

        int soundState = alSndpGetState(&gSoundPlayer);

        if (soundState == AL_STOPPED && (sound->flags & SOUND_HAS_STARTED) != 0) {
            alSndpDeallocate(&gSoundPlayer, sound->soundId);
            sound->soundId = SOUND_ID_NONE;
        } else {
            if (soundState == AL_PLAYING || sound->timeRemaining == 0) {
                sound->flags |= SOUND_HAS_STARTED;
            }

            if (sound->flags & SOUND_FLAGS_3D) {
                float volume;
                float pitch;
                int panning;
                int fxMix;

                soundPlayerDetermine3DSound(&sound->pos3D, &sound->velocity3D, &sound->volume, &volume, &panning, &pitch, &fxMix);

                if (sound->soundType != SoundTypeVoice) {
                    volume *= soundDamping;
                }

                alSndpSetVol(&gSoundPlayer, (short)(32767 * volume));
                alSndpSetPan(&gSoundPlayer, panning);
                alSndpSetPitch(&gSoundPlayer, sound->basePitch * pitch);
                alSndpSetFXMix(&gSoundPlayer, fxMix);
            }

            ++writeIndex;
        }
        
        ++index;

        if (writeIndex != index) {
            gActiveSounds[writeIndex] = gActiveSounds[index];
        }
    }

    soundDamping = mathfMoveTowards(soundDamping, isVoiceActive ? SOUND_DAMPING_LEVEL : 1.0f, FIXED_DELTA_TIME);

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
        activeSound->timeRemaining = 0;
    }
}

void soundPlayerStopAll() {
    for (int i = 0; i < gActiveSoundCount; ++i) {
        struct ActiveSound* activeSound = &gActiveSounds[i];
        if (activeSound->soundId != SOUND_ID_NONE) {
            alSndpSetSound(&gSoundPlayer, activeSound->soundId);
            alSndpStop(&gSoundPlayer);
            activeSound->timeRemaining = 0;
        }
    }
}

void soundPlayerUpdatePosition(ALSndId soundId, struct Vector3* at, struct Vector3* velocity) {
    struct ActiveSound* activeSound = soundPlayerFindActiveSound(soundId);

    if (activeSound) {
        activeSound->flags |= SOUND_FLAGS_3D;
        activeSound->pos3D = *at;
        activeSound->velocity3D = *velocity;
    }
}

float soundPlayerGetOriginalVolume(ALSndId soundId) {
    struct ActiveSound* activeSound = soundPlayerFindActiveSound(soundId);

    if (activeSound) {
        return activeSound->originalVolume;
    }

    return 0.0f;
}

void soundPlayerAdjustVolume(ALSndId soundId, float newVolume) {
    struct ActiveSound* activeSound = soundPlayerFindActiveSound(soundId);

    if (activeSound) {
        activeSound->originalVolume = newVolume;

        newVolume = newVolume * gSaveData.audio.soundVolume / 0xFFFF;
        if (activeSound->soundType == SoundTypeMusic) {
            newVolume = newVolume * gSaveData.audio.musicVolume / 0xFFFF;
        }

        if (activeSound->flags & SOUND_FLAGS_3D) {
            activeSound->volume = newVolume;
        } else {
            short newVolumeInt = (short)(32767 * newVolume);
            short existingVolume = (short)(32767 * activeSound->volume);

            if (newVolumeInt != existingVolume) {
                alSndpSetSound(&gSoundPlayer, activeSound->soundId);
                alSndpSetVol(&gSoundPlayer, newVolumeInt);
                activeSound->volume = newVolume;
            }
        }
    }
}

void soundPlayerFadeOutsideRadius(float volumePercent, struct Vector3* origin, float radius, int persistent) {
    float radiusSquared = radius * radius;

    for (int i = 0; i < gActiveSoundCount; ++i) {
        struct ActiveSound* sound = &gActiveSounds[i];

        if (!sound ||
            sound->soundType != SoundTypeAll ||
            !(sound->flags & SOUND_FLAGS_3D) ||
            vector3DistSqrd(origin, &sound->pos3D) <= radiusSquared
        ) {
            continue;
        }

        float newVolume = volumePercent * sound->originalVolume * (gSaveData.audio.soundVolume / 0xFFFF);
        sound->volume = newVolume;

        if (persistent) {
            sound->originalVolume = newVolume;
        }
    }
}

int soundPlayerIsPlaying(ALSndId soundId) {
    struct ActiveSound* activeSound = soundPlayerFindActiveSound(soundId);

    if (!activeSound) {
        return 0;
    }

    if (!(activeSound->flags & SOUND_HAS_STARTED)) {
        return 1;
    }

    alSndpSetSound(&gSoundPlayer, soundId);
    return activeSound->timeRemaining > 0 && alSndpGetState(&gSoundPlayer) != AL_STOPPED;
}

int soundPlayerIsLoopedById(int soundId){
    struct ActiveSound* activeSound = soundPlayerFindActiveSound(soundId);

    if (!activeSound) {
        return 0;
    }

    if (activeSound->flags & SOUND_FLAGS_LOOPING){
        return 1;
    }

    return 0;
}

void soundListenerUpdate(struct Vector3* position, struct Vector3* right, struct Vector3* velocity, int listenerIndex) {
    gSoundListeners[listenerIndex].worldPos = *position;
    gSoundListeners[listenerIndex].rightVector = *right;
    gSoundListeners[listenerIndex].velocity = *velocity;
}

void soundListenerSetCount(int count) {
    gActiveListenerCount = count;
}

void soundPlayerPause() {
    for (int i = 0; i < gActiveSoundCount; ++i) {
        struct ActiveSound* activeSound = &gActiveSounds[i];
        if (activeSound->soundId != SOUND_ID_NONE) {
            activeSound->flags |= SOUND_FLAGS_PAUSED;

            alSndpSetSound(&gSoundPlayer, activeSound->soundId);
            alSndpSetPitch(&gSoundPlayer, 0.0f);
            alSndpSetVol(&gSoundPlayer, 0);
        }
    }
}

void soundPlayerResume() {
    for (int i = 0; i < gActiveSoundCount; ++i) {
        struct ActiveSound* activeSound = &gActiveSounds[i];
        if (activeSound->flags & SOUND_FLAGS_PAUSED) {
            activeSound->flags &= ~SOUND_FLAGS_PAUSED;

            alSndpSetSound(&gSoundPlayer, activeSound->soundId);
            alSndpSetPitch(&gSoundPlayer, activeSound->basePitch);
            alSndpSetVol(&gSoundPlayer, (short)(32767 * activeSound->volume));
        }
    }
    soundPlayerGameVolumeUpdate();
}
