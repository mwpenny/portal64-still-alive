#include "soundplayer.h"

#include "clips.h"
#include "math/mathf.h"
#include "math/transform.h"
#include "physics/collision_scene.h"
#include "savefile/savefile.h"
#include "system/cartridge.h"
#include "system/time.h"
#include "util/frame_time.h"

#define MAX_SKIPPABLE_SOUNDS    18

#define MAX_ACTIVE_SOUNDS       24

#define SOUND_FLAGS_3D          (1 << 0)
#define SOUND_FLAGS_LOOPING     (1 << 1)
#define SOUND_FLAGS_PAUSED      (1 << 2)

#define SPEED_OF_SOUND          343.2f
#define VOLUME_FADE_THRESHOLD   0.055f
#define VOLUME_FADE_DISTANCE    3.5f
#define VOLUME_CURVE_PAD        0.0125f
#define VOLUME_AMPLIFICATION    1.538f
#define ECHO_VOICE_AMOUNT       0.25f
#define ECHO_SPATIAL_PEAK_DIST  25.0f
#define ECHO_SPATIAL_MIN        0.1f
#define ECHO_SPATIAL_MAX        0.6f
#define ECHO_DEFAULT_AMOUNT     0.0625f  // Small amount of echo for all non-3D sounds

struct ActiveSound {
    SoundId soundId;
    u16 flags;
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

void soundPlayerDetermine3DSound(struct Vector3* at, struct Vector3* velocity, float* volumeIn, float* volumeOut, float* panOut, float* pitchBend, float* echo) {
    *panOut = 0.5f;
    *pitchBend = 1.0f;
    *echo = 0.0f;

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

    // Add echo/reverb amount
    float echoDistCurve = clampf(
        mathfLerp(ECHO_SPATIAL_MIN, ECHO_SPATIAL_MAX, distanceSqrt / ECHO_SPATIAL_PEAK_DIST),
        ECHO_SPATIAL_MIN, ECHO_SPATIAL_MAX
    );
    *echo = echoDistCurve;

    struct Vector3 offset;
    vector3Sub(at, &nearestListener->worldPos, &offset);

    struct Vector3 relativeVelocity;
    vector3Sub(velocity, &nearestListener->velocity, &relativeVelocity);

    float invDist = 1.0f / distanceSqrt;

    float directionalVelocity = -vector3Dot(&offset, &relativeVelocity) * invDist;

    *pitchBend = (SPEED_OF_SOUND + directionalVelocity) * (1.0f / SPEED_OF_SOUND);

    float pan = vector3Dot(&offset, &nearestListener->rightVector) * invDist;
    pan = (pan * 0.5f) + 0.5f;
    *panOut = clampf(pan, 0.0f, 1.0f);
}

void* soundPlayerInit(void* memoryEnd) {
    for (int i = 0; i < MAX_ACTIVE_SOUNDS; ++i) {
        gActiveSounds[i].soundId = SOUND_ID_NONE;
    }

    return audioInit(memoryEnd, MAX_ACTIVE_SOUNDS);
}

SoundId soundPlayerPlay(int soundClipId, float volume, float pitch, struct Vector3* at, struct Vector3* velocity, enum SoundType type) {
    if (gActiveSoundCount >= MAX_ACTIVE_SOUNDS ||
        (gActiveSoundCount >= MAX_SKIPPABLE_SOUNDS && clipsCheckSoundSkippable(soundClipId))
    ) {
        return SOUND_ID_NONE;
    }

    struct ActiveSound* sound = &gActiveSounds[gActiveSoundCount];
    sound->flags = 0;
    sound->originalVolume = volume;
    sound->basePitch = pitch;
    sound->soundType = type;

    volume = (volume * gSaveData.audio.soundVolume) / 0xFFFF;
    if (type == SoundTypeMusic) {
        volume = (volume * gSaveData.audio.musicVolume) / 0xFFFF;
    }
    sound->volume = volume;

    float pan = 0.5f;
    float echo = 0.0f;

    if (at) {
        sound->flags |= SOUND_FLAGS_3D;
        sound->pos3D = *at;
        sound->velocity3D = *velocity;

        float pitchBend;
        soundPlayerDetermine3DSound(at, velocity, &volume, &volume, &pan, &pitchBend, &echo);
        pitch *= pitchBend;
    } else if (type == SoundTypeVoice) {
        echo = ECHO_VOICE_AMOUNT;
    } else if (type == SoundTypeAll) {
        echo = ECHO_VOICE_AMOUNT;
    }

    if (audioIsSoundClipLooped(soundClipId)) {
        sound->flags |= SOUND_FLAGS_LOOPING;
    }

    SoundId soundId = audioPlaySound(
        soundClipId,
        volume,
        pitch,
        pan,
        echo
    );
    if (soundId == SOUND_ID_NONE) {
        return soundId;
    }

    sound->soundId = soundId;
    ++gActiveSoundCount;

    return soundId;
}

void soundPlayerGameVolumeUpdate() {
    for (int i = 0; i < gActiveSoundCount; ++i) {
        struct ActiveSound* sound = &gActiveSounds[i];
        if (sound->soundId == SOUND_ID_NONE) {
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
            float pan;
            float echo;
            soundPlayerDetermine3DSound(&sound->pos3D, &sound->velocity3D, &sound->volume, &volume, &pan, &pitch, &echo);
            audioSetSoundParams(sound->soundId, volume, sound->basePitch * pitch, pan, echo);
        } else {
            audioSetSoundParams(sound->soundId, newVolume, -1.0f, -1.0f, -1.0f);
            sound->volume = newVolume;
        }
    }
}

#define SOUND_DAMPING_LEVEL 0.5f

void soundPlayerUpdate() {
    static float soundDamping = 1.0f;

    int index = 0;
    int writeIndex = 0;
    int isVoiceActive = 0;

    audioUpdate();

    while (index < gActiveSoundCount) {
        struct ActiveSound* sound = &gActiveSounds[index];

        if (sound->flags & SOUND_FLAGS_PAUSED) {
            ++writeIndex;
            ++index;
            continue;
        }

        if (sound->soundType == SoundTypeVoice) {
            isVoiceActive = 1;
        }

        if (!audioIsSoundPlaying(sound->soundId)) {
            audioFreeSound(sound->soundId);
            sound->soundId = SOUND_ID_NONE;
        } else {
            if (sound->flags & SOUND_FLAGS_3D) {
                float volume;
                float pitch;
                float pan;
                float echo;

                soundPlayerDetermine3DSound(&sound->pos3D, &sound->velocity3D, &sound->volume, &volume, &pan, &pitch, &echo);

                if (sound->soundType != SoundTypeVoice) {
                    volume *= soundDamping;
                }

                audioSetSoundParams(sound->soundId, volume, sound->basePitch * pitch, pan, echo);
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

struct ActiveSound* soundPlayerFindActiveSound(SoundId soundId) {
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


void soundPlayerStop(SoundId soundId) {
    struct ActiveSound* activeSound = soundPlayerFindActiveSound(soundId);

    if (activeSound) {
        audioStopSound(soundId);
    }
}

void soundPlayerStopAll() {
    for (int i = 0; i < gActiveSoundCount; ++i) {
        struct ActiveSound* activeSound = &gActiveSounds[i];
        if (activeSound->soundId != SOUND_ID_NONE) {
            audioStopSound(activeSound->soundId);
        }
    }
}

void soundPlayerUpdatePosition(SoundId soundId, struct Vector3* at, struct Vector3* velocity) {
    struct ActiveSound* activeSound = soundPlayerFindActiveSound(soundId);

    if (activeSound) {
        activeSound->flags |= SOUND_FLAGS_3D;
        activeSound->pos3D = *at;
        activeSound->velocity3D = *velocity;
    }
}

float soundPlayerGetOriginalVolume(SoundId soundId) {
    struct ActiveSound* activeSound = soundPlayerFindActiveSound(soundId);

    if (activeSound) {
        return activeSound->originalVolume;
    }

    return 0.0f;
}

void soundPlayerAdjustVolume(SoundId soundId, float newVolume) {
    struct ActiveSound* activeSound = soundPlayerFindActiveSound(soundId);

    if (activeSound) {
        activeSound->originalVolume = newVolume;

        newVolume = newVolume * gSaveData.audio.soundVolume / 0xFFFF;
        if (activeSound->soundType == SoundTypeMusic) {
            newVolume = newVolume * gSaveData.audio.musicVolume / 0xFFFF;
        }

        if (activeSound->flags & SOUND_FLAGS_3D) {
            activeSound->volume = newVolume;
        } else if (newVolume != activeSound->volume) {
            audioSetSoundParams(activeSound->soundId, newVolume, -1.0f, -1.0f, -1.0f);
            activeSound->volume = newVolume;
        }
    }
}

void soundPlayerFadeOutsideRadius(float volumePercent, struct Vector3* origin, float radius, int persistent) {
    float radiusSquared = radius * radius;

    for (int i = 0; i < gActiveSoundCount; ++i) {
        struct ActiveSound* sound = &gActiveSounds[i];

        if (sound->soundId == SOUND_ID_NONE ||
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

int soundPlayerIsPlaying(SoundId soundId) {
    struct ActiveSound* activeSound = soundPlayerFindActiveSound(soundId);
    return activeSound && audioIsSoundPlaying(activeSound->soundId);
}

int soundPlayerIsLooped(SoundId soundId) {
    struct ActiveSound* activeSound = soundPlayerFindActiveSound(soundId);
    return activeSound && (activeSound->flags & SOUND_FLAGS_LOOPING);
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
        if (activeSound->soundId == SOUND_ID_NONE) {
            continue;
        }

        activeSound->flags |= SOUND_FLAGS_PAUSED;
        audioPauseSound(activeSound->soundId);
    }
}

void soundPlayerResume() {
    for (int i = 0; i < gActiveSoundCount; ++i) {
        struct ActiveSound* activeSound = &gActiveSounds[i];
        if (!(activeSound->flags & SOUND_FLAGS_PAUSED)) {
            continue;
        }

        activeSound->flags &= ~SOUND_FLAGS_PAUSED;
        audioResumeSound(activeSound->soundId);
    }
    soundPlayerGameVolumeUpdate();
}
