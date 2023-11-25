#include "audio.h"
#include "soundplayer.h"
#include "soundarray.h"
#include "util/rom.h"
#include "util/time.h"
#include "math/mathf.h"
#include "clips.h"
#include "../physics/collision_scene.h"
#include "../math/transform.h"
#include "../savefile/savefile.h"

struct SoundArray* gSoundClipArray;
ALSndPlayer gSoundPlayer;

#define MAX_SKIPPABLE_SOUNDS    6
#define MAX_ACTIVE_SOUNDS       12

#define SOUND_FLAGS_3D          (1 << 0)
#define SOUND_FLAGS_LOOPING     (1 << 1)
#define SOUND_HAS_STARTED       (1 << 2)
#define SOUND_FLAGS_PAUSED      (1 << 3)

#define SPEED_OF_SOUND          343.2f

struct ActiveSound {
    ALSndId soundId;
    u16 flags;
    float estimatedTimeLeft;
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

void soundPlayerDetermine3DSound(struct Vector3* at, struct Vector3* velocity, float* volumeIn, float* volumeOut, int* panOut, float* pitchBend) {
    if (!gActiveListenerCount) {
        *volumeOut = *volumeIn;
        *panOut = 64;
        return;
    }

    struct SoundListener* nearestListener = &gSoundListeners[0];
    float distance = vector3DistSqrd(at, &gSoundListeners[0].worldPos);
    int through_0_from_1 = 0;
    int through_1_from_0 = 0;


    int portalsPresent = 0;
    struct Transform portal0transform;
    struct Transform portal1transform;
    if (gCollisionScene.portalTransforms[0] != NULL && gCollisionScene.portalTransforms[1] != NULL){
        portalsPresent = 1;
        portal0transform = *gCollisionScene.portalTransforms[0];
        portal1transform = *gCollisionScene.portalTransforms[1];
    }

    for (int i = 0; i < MAX_SOUND_LISTENERS; ++i) {
        float check = vector3DistSqrd(at, &gSoundListeners[i].worldPos);
        if (check < distance) {
            distance = check;
            nearestListener = &gSoundListeners[i];
            through_0_from_1 = 0;
            through_1_from_0 = 0;
        }

        if (portalsPresent){
            float dist1,dist2;
            // check dist from obj to 0 portal + from 1 portal to listener
            dist1 =  vector3DistSqrd(at, &portal0transform.position);
            dist2 =  vector3DistSqrd(&portal1transform.position, &gSoundListeners[i].worldPos);
            if ((dist1+dist2) < distance){
                distance = (dist1+dist2);
                nearestListener = &gSoundListeners[i];
                through_0_from_1 = 1;
                through_1_from_0 = 0;
            }
            // check dist from obj to 1 portal + from 0 portal to listener
            dist1 =  vector3DistSqrd(at, &portal1transform.position);
            dist2 =  vector3DistSqrd(&portal0transform.position, &gSoundListeners[i].worldPos);
            if ((dist1+dist2) < distance){
                distance = (dist1+dist2);
                nearestListener = &gSoundListeners[i];
                through_0_from_1 = 0;
                through_1_from_0 = 1;
            }
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
    if (through_0_from_1){
        vector3Sub(&portal1transform.position, &nearestListener->worldPos, &offset);
    }
    else if(through_1_from_0){
        vector3Sub(&portal0transform.position, &nearestListener->worldPos, &offset);
    }
    else{
        vector3Sub(at, &nearestListener->worldPos, &offset);
    }
    
    struct Vector3 relativeVelocity;
    vector3Sub(velocity, &nearestListener->velocity, &relativeVelocity);

    float invDist = 1.0f / sqrtf(distance);

    float directionalVelocity = -vector3Dot(&offset, &relativeVelocity) * invDist;

    *pitchBend = (SPEED_OF_SOUND + directionalVelocity) * (1.0f / SPEED_OF_SOUND);

    float pan = vector3Dot(&offset, &nearestListener->rightVector) * invDist;

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
    sound->estimatedTimeLeft = soundPlayerEstimateLength(alSound, pitch);
    sound->volume = volume;
    sound->originalVolume = volume;
    sound->basePitch = pitch;
    sound->soundType = type;

    float newVolume = sound->originalVolume * gSaveData.audio.soundVolume/0xFFFF;
    if (type == SoundTypeMusic){
        newVolume = newVolume * gSaveData.audio.musicVolume/0xFFFF;
    }
    sound->volume = newVolume;

    int panning = 64;

    if (at) {
        sound->flags |= SOUND_FLAGS_3D;
        sound->pos3D = *at;
        sound->velocity3D = *velocity;
        float pitchBend;
        soundPlayerDetermine3DSound(at, velocity, &newVolume, &newVolume, &panning, &pitchBend);
        pitch = pitch * pitchBend;
    }

    if (soundPlayerIsLooped(alSound)) {
        sound->flags |= SOUND_FLAGS_LOOPING;
    }

    alSndpSetSound(&gSoundPlayer, result);
    alSndpSetVol(&gSoundPlayer, (short)(32767 * newVolume));
    alSndpSetPitch(&gSoundPlayer, pitch);
    alSndpSetPan(&gSoundPlayer, panning);
    alSndpPlay(&gSoundPlayer);

    ++gActiveSoundCount;

    return result;
}

float soundClipDuration(int soundClipId, float pitch) {
    if (soundClipId < 0 || soundClipId >= gSoundClipArray->soundCount) {
        return 0.0f;
    }

    ALSound* alSound = gSoundClipArray->sounds[soundClipId];
    return soundPlayerEstimateLength(alSound, pitch);
}

void soundPlayerGameVolumeUpdate() {
    int index = 0;
    while (index < gActiveSoundCount) {
        struct ActiveSound* sound = &gActiveSounds[index];
        if (!sound){
            ++index;
            continue;
        }

        float newVolume = sound->originalVolume * gSaveData.audio.soundVolume/0xFFFF;
        if (sound->soundType == SoundTypeMusic){
            newVolume = newVolume* gSaveData.audio.musicVolume/0xFFFF;
        }
        
        if (sound->flags & SOUND_FLAGS_PAUSED) {
            sound->volume = newVolume;
            ++index;
            continue;
        }
        if (sound->flags & SOUND_FLAGS_3D){
            sound->volume = newVolume;
            float volume;
            float pitch;
            int panning;
            soundPlayerDetermine3DSound(&sound->pos3D, &sound->velocity3D, &sound->volume, &volume, &panning, &pitch);
            alSndpSetSound(&gSoundPlayer, sound->soundId);
            alSndpSetVol(&gSoundPlayer, (short)(32767 * volume));
            alSndpSetPan(&gSoundPlayer, panning);
            alSndpSetPitch(&gSoundPlayer, sound->basePitch * pitch);
            ++index;
            continue;
            
        } else {
            alSndpSetSound(&gSoundPlayer, sound->soundId);
            alSndpSetVol(&gSoundPlayer, (short)(32767 * newVolume));
            sound->volume = newVolume;
            ++index;
            continue;
        }
    }
}

#define SOUND_DAMPING_LEVEL 0.5f

void soundPlayerUpdate() {
    int index = 0;
    int writeIndex = 0;
    int isVoiceActive = 0;
    static float soundDamping = 1.0f;

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

        sound->estimatedTimeLeft -= FIXED_DELTA_TIME;

        alSndpSetSound(&gSoundPlayer, sound->soundId);

        int soundState = alSndpGetState(&gSoundPlayer);

        if (soundState == AL_STOPPED && (sound->flags & SOUND_HAS_STARTED) != 0) {
            alSndpDeallocate(&gSoundPlayer, sound->soundId);
            sound->soundId = SOUND_ID_NONE;
        } else {
            if (soundState == AL_PLAYING || sound->estimatedTimeLeft < 0.0) {
                sound->flags |= SOUND_HAS_STARTED;
            }

            if (sound->flags & SOUND_FLAGS_3D) {
                float volume;
                float pitch;
                int panning;

                soundPlayerDetermine3DSound(&sound->pos3D, &sound->velocity3D, &sound->volume, &volume, &panning, &pitch);

                if (sound->soundType != SoundTypeVoice) {
                    volume *= soundDamping;
                }

                alSndpSetVol(&gSoundPlayer, (short)(32767 * volume));
                alSndpSetPan(&gSoundPlayer, panning);
                alSndpSetPitch(&gSoundPlayer, sound->basePitch * pitch);
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
        activeSound->estimatedTimeLeft = 0.0f;
    }
}

void soundPlayerStopAll() {
    for (int i = 0; i < gActiveSoundCount; ++i) {
        struct ActiveSound* activeSound = &gActiveSounds[i];
        if (activeSound->soundId != SOUND_ID_NONE) {
            alSndpSetSound(&gSoundPlayer, activeSound->soundId);
            alSndpStop(&gSoundPlayer);
            activeSound->estimatedTimeLeft = 0.0f;
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

void soundPlayerAdjustVolume(ALSndId soundId, float newVolume) {
    struct ActiveSound* activeSound = soundPlayerFindActiveSound(soundId);

    if (activeSound) {
        newVolume = newVolume * gSaveData.audio.soundVolume/0xFFFF;
        if (activeSound->soundType == SoundTypeMusic){
            newVolume = newVolume * gSaveData.audio.musicVolume/0xFFFF;
        }
        if (activeSound->flags & SOUND_FLAGS_3D){
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

int soundPlayerIsPlaying(ALSndId soundId) {
    struct ActiveSound* activeSound = soundPlayerFindActiveSound(soundId);

    if (!activeSound) {
        return 0;
    }

    if (!(activeSound->flags & SOUND_HAS_STARTED)) {
        return 1;
    }

    alSndpSetSound(&gSoundPlayer, soundId);
    return activeSound->estimatedTimeLeft > 0.0f && alSndpGetState(&gSoundPlayer) != AL_STOPPED;
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

float soundPlayerTimeLeft(ALSndId soundId) {
    struct ActiveSound* activeSound = soundPlayerFindActiveSound(soundId);

    if (!activeSound) {
        return 0.0f;
    }

    return activeSound->estimatedTimeLeft;
}

void soundListenerUpdate(struct Vector3* position, struct Quaternion* rotation, struct Vector3* velocity, int listenerIndex) {
    gSoundListeners[listenerIndex].worldPos = *position;
    gSoundListeners[listenerIndex].velocity = *velocity;
    quatMultVector(rotation, &gRight, &gSoundListeners[listenerIndex].rightVector);
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