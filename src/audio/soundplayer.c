#include "soundplayer.h"

#include "clips.h"
#include "math/mathf.h"
#include "savefile/savefile.h"
#include "util/frame_time.h"

#define MAX_SOUND_LISTENERS     3

#define SOUND_FLAGS_3D          (1 << 0)

#define VOLUME_FADE_THRESHOLD   0.055f
#define VOLUME_FADE_LENGTH      3.5f
#define VOLUME_CURVE_PAD        0.0125f
#define VOLUME_AMPLIFICATION    1.538f
#define VOLUME_VOICE_DAMPING    0.5f

#define SPEED_OF_SOUND          343.2f

#define ECHO_SPATIAL_PEAK_DIST  25.0f
#define ECHO_SPATIAL_MIN        0.1f
#define ECHO_SPATIAL_MAX        0.6f
#define ECHO_VOICE_AMOUNT       0.25f
#define ECHO_DEFAULT_AMOUNT     0.0625f

struct Sound {
    SoundId soundId;
    u8 flags;
    struct Vector3 pos3D;
    struct Vector3 velocity3D;
    float originalVolume;
    float volumePercent;
    float basePitch;
    enum SoundType type;
};

struct SoundListener {
    struct Vector3 worldPos;
    struct Vector3 rightVector;
    struct Vector3 velocity;
};

static struct Sound sSounds[MAX_ACTIVE_SOUNDS];
static int sActiveSoundCount = 0;

static struct SoundListener sSoundListeners[MAX_SOUND_LISTENERS];
static int sActiveListenerCount = 0;

static float sGameVolumePercent = 1.0f;
static float sMusicVolumePercent = 0.5f;

static struct Sound* soundPlayerFindActiveSound(SoundId soundId) {
    if (soundId == SOUND_ID_NONE) {
        return NULL;
    }

    for (int i = 0; i < sActiveSoundCount; ++i) {
        if (sSounds[i].soundId == soundId) {
            return &sSounds[i];
        }
    }

    return NULL;
}

static void soundPlayerSetVolumePercent(struct Sound* sound) {
    sound->volumePercent = (sound->type == SoundTypeMusic) ? sMusicVolumePercent : sGameVolumePercent;
}

static void soundPlayerCalc3DSoundParams(struct Sound* sound, float* volume, float* pitchBend, float* pan, float* echo) {
    *pitchBend = 1.0f;
    *pan = 0.5f;
    *echo = 0.0f;

    if (sActiveListenerCount == 0) {
        *volume = sound->originalVolume;
        return;
    }

    // Find nearest listener
    struct SoundListener* listener = NULL;
    float listenerDist = 0.0f;

    for (int i = 0; i < sActiveListenerCount; ++i) {
        float dist = vector3DistSqrd(&sound->pos3D, &sSoundListeners[i].worldPos);

        if (!listener || dist < listenerDist) {
            listener = &sSoundListeners[i];
            listenerDist = dist;
        }
    }

    if (listenerDist < 0.0000001f) {
        *volume = sound->originalVolume;
        return;
    }

    listenerDist = sqrtf(listenerDist);

    // Initial linear volume level
    float newVolume = clampf(sound->originalVolume / listenerDist, 0.0f, 1.0f);

    // Fade quiet sounds more aggressively
    if (newVolume > 0.0f && newVolume < VOLUME_FADE_THRESHOLD) {
        // How far is the listener from where the threshold volume was reached?
        float fadeStartDist = sound->originalVolume * (1.0f / VOLUME_FADE_THRESHOLD);
        float distFromFadeStart = listenerDist - fadeStartDist;

        float fadeAmount = clampf(distFromFadeStart * (1.0f / VOLUME_FADE_LENGTH), 0.0f, 1.0f);
        newVolume *= (1.0f - fadeAmount);
    }

    // Fudge with the volume curve a bit
    // Try to make distant sounds more apparent while compressing the volume of closer sounds
    *volume = clampf((newVolume - VOLUME_CURVE_PAD) * VOLUME_AMPLIFICATION, 0.0f, 1.0f);
    if (*volume == 0.0f) {
        return;
    }

    // Doppler effect
    struct Vector3 listenerOffset;
    vector3Sub(&sound->pos3D, &listener->worldPos, &listenerOffset);

    struct Vector3 relativeVelocity;
    vector3Sub(&sound->velocity3D, &listener->velocity, &relativeVelocity);

    float invDist = 1.0f / listenerDist;
    float directionalVelocity = -vector3Dot(&listenerOffset, &relativeVelocity) * invDist;
    *pitchBend = (SPEED_OF_SOUND + directionalVelocity) * (1.0f / SPEED_OF_SOUND);

    // Pan
    float panAmount = vector3Dot(&listenerOffset, &listener->rightVector) * invDist;
    panAmount = (panAmount * 0.5f) + 0.5f;
    *pan = clampf(panAmount, 0.0f, 1.0f);

    // Echo/reverb
    *echo = clampf(
        mathfLerp(ECHO_SPATIAL_MIN, ECHO_SPATIAL_MAX, listenerDist / ECHO_SPATIAL_PEAK_DIST),
        ECHO_SPATIAL_MIN, ECHO_SPATIAL_MAX
    );
}

void* soundPlayerInit(void* memoryEnd) {
    for (int i = 0; i < MAX_ACTIVE_SOUNDS; ++i) {
        sSounds[i].soundId = SOUND_ID_NONE;
    }

    sActiveSoundCount = 0;
    sActiveListenerCount = 0;
    soundPlayerUpdateVolumeLevels();

    return audioInit(memoryEnd, MAX_ACTIVE_SOUNDS);
}

void soundPlayerUpdate() {
    static float soundDamping = 1.0f;

    int index = 0;
    int writeIndex = 0;
    int isVoiceActive = 0;

    audioUpdate();

    while (index < sActiveSoundCount) {
        struct Sound* sound = &sSounds[index];

        if (audioIsSoundPaused(sound->soundId)) {
            ++writeIndex;
            ++index;
            continue;
        }

        if (sound->type == SoundTypeVoice) {
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
                soundPlayerCalc3DSoundParams(sound, &volume, &pitch, &pan, &echo);

                if (sound->type != SoundTypeVoice) {
                    volume *= soundDamping;
                }

                audioSetSoundParams(sound->soundId, volume * sound->volumePercent, sound->basePitch * pitch, pan, echo);
            }

            ++writeIndex;
        }
        
        ++index;

        if (writeIndex != index) {
            sSounds[writeIndex] = sSounds[index];
        }
    }

    soundDamping = mathfMoveTowards(
        soundDamping,
        isVoiceActive ? VOLUME_VOICE_DAMPING : 1.0f,
        FIXED_DELTA_TIME
    );
    sActiveSoundCount = writeIndex;
}

int soundPlayerSoundCount() {
    return sActiveSoundCount;
}

SoundId soundPlayerPlay(int soundClipId, float volume, float pitch, struct Vector3* position, struct Vector3* velocity, enum SoundType type) {
    if (sActiveSoundCount >= MAX_ACTIVE_SOUNDS ||
        (sActiveSoundCount >= MAX_SKIPPABLE_SOUNDS && clipsCheckSoundSkippable(soundClipId))
    ) {
        return SOUND_ID_NONE;
    }

    struct Sound* sound = &sSounds[sActiveSoundCount];
    sound->flags = 0;
    sound->originalVolume = volume;
    sound->basePitch = pitch;
    sound->type = type;
    soundPlayerSetVolumePercent(sound);

    float pan = 0.5f;
    float echo = 0.0f;
    volume = sound->originalVolume;

    if (position) {
        sound->flags |= SOUND_FLAGS_3D;
        sound->pos3D = *position;
        sound->velocity3D = *velocity;

        float pitchBend;
        soundPlayerCalc3DSoundParams(sound, &volume, &pitchBend, &pan, &echo);
        pitch *= pitchBend;
    } else if (type == SoundTypeVoice) {
        echo = ECHO_VOICE_AMOUNT;
    } else if (type == SoundTypeAll) {
        echo = ECHO_DEFAULT_AMOUNT;
    }

    SoundId soundId = audioPlaySound(
        soundClipId,
        volume * sound->volumePercent,
        pitch,
        pan,
        echo
    );
    if (soundId == SOUND_ID_NONE) {
        return soundId;
    }

    sound->soundId = soundId;
    ++sActiveSoundCount;

    return soundId;
}

int soundPlayerIsPlaying(SoundId soundId) {
    struct Sound* sound = soundPlayerFindActiveSound(soundId);
    return sound && audioIsSoundPlaying(sound->soundId);
}

int soundPlayerIsLooped(SoundId soundId) {
    struct Sound* sound = soundPlayerFindActiveSound(soundId);
    return sound && audioIsSoundLooped(sound->soundId);
}

int soundPlayerIsMuted(SoundId soundId) {
    struct Sound* sound = soundPlayerFindActiveSound(soundId);
    return !sound || sound->originalVolume == 0.0f;
}

void soundPlayerSetPosition(SoundId soundId, struct Vector3* position, struct Vector3* velocity) {
    struct Sound* sound = soundPlayerFindActiveSound(soundId);
    if (!sound) {
        return;
    }

    sound->flags |= SOUND_FLAGS_3D;
    sound->pos3D = *position;
    sound->velocity3D = *velocity;
}

void soundPlayerSetVolume(SoundId soundId, float newVolume) {
    struct Sound* sound = soundPlayerFindActiveSound(soundId);
    if (!sound || sound->originalVolume == newVolume) {
        return;
    }

    sound->originalVolume = newVolume;

    // 3D sound volume will be updated in next call to soundPlayerUpdate()
    if (!(sound->flags & SOUND_FLAGS_3D)) {
        audioSetSoundParams(sound->soundId, sound->originalVolume * sound->volumePercent, -1.0f, -1.0f, -1.0f);
    }
}

void soundPlayerStop(SoundId soundId) {
    struct Sound* sound = soundPlayerFindActiveSound(soundId);
    if (sound) {
        audioStopSound(soundId);
    }
}

void soundPlayerStopAll() {
    for (int i = 0; i < sActiveSoundCount; ++i) {
        struct Sound* sound = &sSounds[i];
        if (sound->soundId != SOUND_ID_NONE) {
            audioStopSound(sound->soundId);
        }
    }
}

void soundPlayerPause() {
    for (int i = 0; i < sActiveSoundCount; ++i) {
        struct Sound* sound = &sSounds[i];
        if (sound->soundId != SOUND_ID_NONE) {
            audioPauseSound(sound->soundId);
        }
    }
}

void soundPlayerResume() {
    for (int i = 0; i < sActiveSoundCount; ++i) {
        struct Sound* sound = &sSounds[i];
        if (sound->soundId != SOUND_ID_NONE && audioIsSoundPaused(sound->soundId)) {
            audioResumeSound(sound->soundId);
        }
    }
}

void soundPlayerFadeOutsideRadius(float volumePercent, struct Vector3* origin, float radius, int persistent) {
    float radiusSquared = radius * radius;

    for (int i = 0; i < sActiveSoundCount; ++i) {
        struct Sound* sound = &sSounds[i];

        if (sound->soundId == SOUND_ID_NONE ||
            sound->type != SoundTypeAll ||
            !(sound->flags & SOUND_FLAGS_3D) ||
            vector3DistSqrd(origin, &sound->pos3D) <= radiusSquared
        ) {
            continue;
        }

        soundPlayerSetVolumePercent(sound);
        sound->volumePercent *= volumePercent;

        if (persistent) {
            sound->originalVolume *= sound->volumePercent;
        }
    }
}

void soundPlayerUpdateVolumeLevels() {
    sGameVolumePercent = (float)gSaveData.audio.soundVolume / 0xFFFF;
    sMusicVolumePercent = (sGameVolumePercent * gSaveData.audio.musicVolume) / 0xFFFF;

    for (int i = 0; i < sActiveSoundCount; ++i) {
        struct Sound* sound = &sSounds[i];
        if (sound->soundId == SOUND_ID_NONE) {
            continue;
        }

        soundPlayerSetVolumePercent(sound);

        float volume = sound->originalVolume;

        // Don't wait for soundPlayerUpdate(), to avoid harsh transitions on config change
        if (sound->flags & SOUND_FLAGS_3D) {
            float _pitch;
            float _pan;
            float _echo;
            soundPlayerCalc3DSoundParams(sound, &volume, &_pitch, &_pan, &_echo);
        }

        audioSetSoundParams(sound->soundId, volume * sound->volumePercent, -1.0f, -1.0f, -1.0f);
    }
}

void soundListenerUpdate(int listenerIndex, struct Vector3* position, struct Vector3* right, struct Vector3* velocity) {
    struct SoundListener* listener = &sSoundListeners[listenerIndex];
    listener->worldPos = *position;
    listener->rightVector = *right;
    listener->velocity = *velocity;
}

void soundListenerSetCount(int count) {
    sActiveListenerCount = count;
}
