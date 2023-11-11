#include "cutscene_runner.h"
#include "../audio/soundplayer.h"
#include "../util/time.h"
#include "../scene/scene.h"
#include "../scene/signals.h"
#include "../levels/levels.h"
#include "../util/memory.h"
#include "../savefile/checkpoint.h"
#include "../locales/locales.h"
#include "../controls/rumble_pak.h"

#include <math.h>

unsigned char gPortalOpenRumbleData[] = {
    0xFA, 0xA9,
};

struct RumblePakWave gPortalOpenRumbleWave = {
    .samples = gPortalOpenRumbleData,
    .sampleCount = 8,
    .samplesPerTick = 1 << 5,
};

#define RUMBLE_PLAYER_DISTANCE  8.0f

struct CutsceneRunner* gRunningCutscenes;
struct CutsceneRunner* gUnusedRunners;
u64 gTriggeredCutscenes;

#define MAX_QUEUE_LENGTH    25

struct QueuedSound {
    struct QueuedSound* next;
    u16 soundId;
    u16 subtitleId;
    float volume;
};

struct QueuedSound gCutsceneSoundNodes[MAX_QUEUE_LENGTH];
struct QueuedSound* gCutsceneNextFreeSound;

struct QueuedSound* gCutsceneSoundQueues[CH_COUNT];
ALSndId gCutsceneCurrentSound[CH_COUNT];
u16 gCutsceneCurrentSoundId[CH_COUNT];
u16 gCutsceneCurrentSubtitleId[CH_COUNT];
float   gCutsceneCurrentVolume[CH_COUNT];


float gCutsceneChannelPitch[CH_COUNT] = {
    [CH_GLADOS] = 0.5f,
    [CH_MUSIC] = 0.5f,
    [CH_AMBIENT] = 0.5f
};

void cutsceneRunnerCancel(struct CutsceneRunner* runner);

void cutsceneRunnerReset() {
    gRunningCutscenes = NULL;
    gTriggeredCutscenes = 0;

    for (int i = 0; i < MAX_QUEUE_LENGTH; ++i) {
        gCutsceneSoundNodes[i].next = (i + 1) < MAX_QUEUE_LENGTH ? &gCutsceneSoundNodes[i + 1] : NULL;
    }

    gCutsceneNextFreeSound = &gCutsceneSoundNodes[0];

    for (int i = 0; i < CH_COUNT; ++i) {
        gCutsceneSoundQueues[i] = NULL;
        gCutsceneCurrentSound[i] = SOUND_ID_NONE;
        gCutsceneCurrentSoundId[i] = SOUND_ID_NONE;
        gCutsceneCurrentSubtitleId[i] = SubtitleKeyNone;
        gCutsceneCurrentVolume[i] = 0.0f;
    }

    struct CutsceneRunner* current = gRunningCutscenes;


    while (current) {
        cutsceneRunnerCancel(current);
        current = current->nextRunner;
    }

    gRunningCutscenes = NULL;
    // the heap is reset on level transition so there
    // is no need to free any cutscene runners
    gUnusedRunners = NULL;
}

struct CutsceneRunner* cutsceneRunnerNew() {
    struct CutsceneRunner* result;

    if (gUnusedRunners) {
        result = gUnusedRunners;
        gUnusedRunners = result->nextRunner;
    } else {
        result = malloc(sizeof(struct CutsceneRunner));
    }

    result->nextRunner = NULL;
    result->currentCutscene = NULL;
    result->currentStep = 0;

    return result;
}

void cutsceneRunnerCancel(struct CutsceneRunner* runner) {
    struct CutsceneStep* step = &runner->currentCutscene->steps[runner->currentStep];

    switch (step->type) {
        case CutsceneStepTypePlaySound:
        case CutsceneStepTypeStartSound:
            soundPlayerStop(runner->state.playSound.soundId);
            break;
        default:
    }

    runner->currentStep = 0;
    runner->currentCutscene = NULL;
}

void cutsceneQueueSound(int soundId, float volume, int channel, int subtitleId) {
    struct QueuedSound* next = gCutsceneNextFreeSound;

    if (!next) {
        return;
    }

    gCutsceneNextFreeSound = gCutsceneNextFreeSound->next;

    next->next = NULL;
    next->soundId = soundId;
    next->volume = volume;
    next->subtitleId = subtitleId;

    struct QueuedSound* tail = gCutsceneSoundQueues[channel];

    while (tail && tail->next) {
        tail = tail->next;
    }

    if (tail) {
        tail->next = next;
    } else {
        gCutsceneSoundQueues[channel] = next;
    }
}

float cutsceneRunnerConvertPlaybackSpeed(s8 asInt) {
    return asInt * (1.0f / 127.0f);
}

unsigned char gCutsceneRumbleSoftData[] = {
    0xAA, 0x90,
};

unsigned char gCutsceneRumbleMediumData[] = {
    0xAA, 0xA9, 0x90,
};

unsigned char gCutsceneRumbleHardData[] = {
    0xFF, 0xAA, 0xAA, 0x99
};

#define CUTSCENE_RUMBLE_CLIP_COUNT   3

struct RumblePakWave gCutsceneRumbleWaves[CUTSCENE_RUMBLE_CLIP_COUNT] = {
    {
        .samples = gCutsceneRumbleSoftData,
        .sampleCount = 6,
        .samplesPerTick = 1 << 5,
    },
    {
        .samples = gCutsceneRumbleMediumData,
        .sampleCount = 10,
        .samplesPerTick = 1 << 5,
    },
    {
        .samples = gCutsceneRumbleHardData,
        .sampleCount = 16,
        .samplesPerTick = 1 << 5,
    },
};

void cutsceneRunnerStartStep(struct CutsceneRunner* runner) {
    struct CutsceneStep* step = &runner->currentCutscene->steps[runner->currentStep];
    
    switch (step->type) {
        case CutsceneStepTypePlaySound:
        case CutsceneStepTypeStartSound:
            runner->state.playSound.soundId = soundPlayerPlay(
                step->playSound.soundId,
                step->playSound.volume * (1.0f / 255.0f),
                step->playSound.pitch * (1.0f / 64.0f),
                NULL,
                NULL,
                SoundTypeAll
            );
            break;
        case CutsceneStepTypeQueueSound:
        {
            cutsceneQueueSoundInChannel(step->queueSound.soundId, step->queueSound.volume * (1.0f / 255.0f), step->queueSound.channel, step->queueSound.subtitleId);
            break;
        }
        case CutsceneStepTypeDelay:
            runner->state.delay = step->delay;
            break;
        case CutsceneStepTypeOpenPortal:
        {
            struct Location* location = &gCurrentLevel->locations[step->openPortal.locationIndex];

            if (step->openPortal.fromPedestal && gCurrentLevel->pedestalCount) {
                struct Vector3 fireFrom = gScene.pedestals[0].transform.position;
                fireFrom.y += 0.75f;
                portalGunFireWorld(&gScene.portalGun, step->openPortal.portalIndex, &fireFrom, &location->transform.position, gScene.pedestals[0].roomIndex);
            } else {
                struct Ray firingRay;
                struct Vector3 transformUp;
                firingRay.origin = location->transform.position;
                quatMultVector(&location->transform.rotation, &gForward, &firingRay.dir);
                quatMultVector(&location->transform.rotation, &gRight, &transformUp);
                vector3Negate(&transformUp, &transformUp);
                vector3AddScaled(&location->transform.position, &firingRay.dir, -0.1f, &firingRay.origin);
                sceneFirePortal(&gScene, &firingRay, &transformUp, step->openPortal.portalIndex, location->roomIndex, 0, 0);

                if (vector3DistSqrd(&location->transform.position, &gScene.player.lookTransform.position) < RUMBLE_PLAYER_DISTANCE * RUMBLE_PLAYER_DISTANCE) {
                    gScene.player.shakeTimer = 0.5f;
                    rumblePakClipPlay(&gPortalOpenRumbleWave);
                }
            }
            break;
        }
        case CutsceneStepTypeClosePortal:
        {
            sceneClosePortal(&gScene, step->closePortal.portalIndex);
            break;
        }
        case CutsceneStepShowPrompt:
        {
            hudShowActionPrompt(&gScene.hud, step->showPrompt.actionPromptType);
            break;
        }
        case CutsceneStepTypeSetSignal:
            signalsSetDefault(step->setSignal.signalIndex, step->setSignal.signalValue);
            break;
        case CutsceneStepTypeTeleportPlayer:
            rigidBodyTeleport(
                &gScene.player.body, 
                &gCurrentLevel->locations[step->teleportPlayer.fromLocation].transform, 
                &gCurrentLevel->locations[step->teleportPlayer.toLocation].transform,
                &gZeroVec,
                &gZeroVec,
                gCurrentLevel->locations[step->teleportPlayer.toLocation].roomIndex
            );
            sceneQueueCheckpoint(&gScene);
            break;
        case CutsceneStepTypeLoadLevel:
        {
            struct Transform exitInverse;
            transformInvert(&gCurrentLevel->locations[step->loadLevel.fromLocation].transform, &exitInverse);
            struct Transform relativeExit;
            struct Vector3 relativeVelocity;

            transformConcat(&exitInverse, &gScene.player.lookTransform, &relativeExit);
            quatMultVector(&exitInverse.rotation, &gScene.player.body.velocity, &relativeVelocity);
            levelQueueLoad(step->loadLevel.levelIndex, &relativeExit, &relativeVelocity);
            break;
        }
        case CutsceneStepTypeGoto:
            runner->currentStep += step->gotoStep.relativeInstructionIndex;
            cutsceneRunnerStartStep(runner);
            break;
        case CutsceneStepTypeStartCutscene:
            cutsceneStart(&gCurrentLevel->cutscenes[step->cutscene.cutsceneIndex]);
            break;
        case CutsceneStepTypeStopCutscene:
            cutsceneStop(&gCurrentLevel->cutscenes[step->cutscene.cutsceneIndex]);
            break;
        case CutsceneStepTypeHidePedestal:
            for (unsigned i = 0; i < gScene.pedestalCount; ++i) {
                pedestalHide(&gScene.pedestals[i]);
            }

            if (!(gScene.player.flags & PlayerHasFirstPortalGun)) {
                playerGivePortalGun(&gScene.player, PlayerHasFirstPortalGun);
            } else if (!(gScene.player.flags & PlayerHasSecondPortalGun)) {
                playerGivePortalGun(&gScene.player, PlayerHasSecondPortalGun);
            }

            break;
        case CutsceneStepTypePointPedestal:
            for (unsigned i = 0; i < gScene.pedestalCount; ++i) {
                pedestalPointAt(&gScene.pedestals[i], &gCurrentLevel->locations[step->pointPedestal.atLocation].transform.position);
            }
            break;
        case CutsceneStepPlayAnimation:
            sceneAnimatorPlay(
                &gScene.animator, 
                step->playAnimation.armatureIndex, 
                step->playAnimation.animationIndex,
                cutsceneRunnerConvertPlaybackSpeed(step->playAnimation.playbackSpeed),
                0
            );
            break;
        case CutsceneStepSetAnimationSpeed:
            sceneAnimatorSetSpeed(
                &gScene.animator,
                step->playAnimation.armatureIndex, 
                cutsceneRunnerConvertPlaybackSpeed(step->playAnimation.playbackSpeed)
            );
            break;
        case CutsceneStepSaveCheckpoint:
            sceneQueueCheckpoint(&gScene);
            break;
        case CutsceneStepKillPlayer:
            playerKill(&gScene.player, step->killPlayer.isWater);
            break;
        case CutsceneStepRumble:
            rumblePakClipPlay(&gCutsceneRumbleWaves[step->rumble.rumbleLevel]);
            break;
        default:
    }
}

int cutsceneRunnerIsChannelPlaying(int channel) {
    return soundPlayerIsPlaying(gCutsceneCurrentSound[channel]) || gCutsceneSoundQueues[channel] != NULL;
}

int cutsceneRunnerUpdateCurrentStep(struct CutsceneRunner* runner) {
    struct CutsceneStep* step = &runner->currentCutscene->steps[runner->currentStep];
    switch (step->type) {
        case CutsceneStepTypePlaySound:
            return soundPlayerIsLoopedById(runner->state.playSound.soundId) || !soundPlayerIsPlaying(runner->state.playSound.soundId);
        case CutsceneStepTypeWaitForChannel:
        {
            int result = !cutsceneRunnerIsChannelPlaying(step->waitForChannel.channel);
            return result;
        }
        case CutsceneStepTypeDelay:
            runner->state.delay -= FIXED_DELTA_TIME;
            return runner->state.delay <= 0.0f;
        case CutsceneStepTypeWaitForSignal:
            return signalsRead(step->waitForSignal.signalIndex);
        case CutsceneStepTypeWaitForCutscene:
            return !cutsceneIsRunning(&gCurrentLevel->cutscenes[step->cutscene.cutsceneIndex]);
        case CutsceneStepWaitForAnimation:
            return !sceneAnimatorIsRunning(&gScene.animator, step->waitForAnimation.armatureIndex);
        default:
            return 1;
    }
}

float cutsceneSoundQueueTime(int channel);

float cutsceneStepEstimateTime(struct CutsceneStep* step, union CutsceneStepState* state) {
    switch (step->type) {
        case CutsceneStepTypePlaySound:
            return state ? soundPlayerTimeLeft(state->playSound.soundId) : soundClipDuration(step->playSound.soundId, step->playSound.pitch * (1.0f / 64.0f));
        case CutsceneStepTypeQueueSound:
            return state ? soundPlayerTimeLeft(state->playSound.soundId) : soundClipDuration(step->queueSound.soundId, gCutsceneChannelPitch[step->queueSound.channel]);
        case CutsceneStepTypeWaitForChannel:
        {
            return state ? cutsceneSoundQueueTime(step->waitForChannel.channel) : 0.0f;
        }
        case CutsceneStepTypeDelay:
            return state ? state->delay : step->delay;
        case CutsceneStepTypeWaitForSignal:
            return 0.0f;
        case CutsceneStepTypeWaitForCutscene:
            return cutsceneEstimateTimeLeft(&gCurrentLevel->cutscenes[step->cutscene.cutsceneIndex]);
        case CutsceneStepWaitForAnimation:
            // Maybe todo
            return 0.0f;
        default:
            return 0.0f;
    }
} 

void cutsceneRunnerRun(struct CutsceneRunner* runner, struct Cutscene* cutscene) {
    runner->currentCutscene = cutscene;
    runner->currentStep = 0;
    cutsceneRunnerStartStep(runner);
}

int cutsceneRunnerIsRunning(struct CutsceneRunner* runner) {
    return runner->currentStep < runner->currentCutscene->stepCount;
}

void cutsceneRunnerUpdate(struct CutsceneRunner* runner) {
    while (cutsceneRunnerIsRunning(runner) && cutsceneRunnerUpdateCurrentStep(runner)) {
        runner->currentStep++;

        if (cutsceneRunnerIsRunning(runner)) {
            cutsceneRunnerStartStep(runner);
        }
    }
}

void cutsceneStart(struct Cutscene* cutscene) {
    struct CutsceneRunner* runner = cutsceneRunnerNew();
    runner->nextRunner = gRunningCutscenes;
    gRunningCutscenes = runner;
    cutsceneRunnerRun(runner, cutscene);
}

void cutsceneStop(struct Cutscene* cutscene) {
    struct CutsceneRunner* previousCutscene = NULL;
    struct CutsceneRunner* current = gRunningCutscenes;

    while (current) {
        if (current->currentCutscene == cutscene) {
            struct CutsceneRunner* toRemove = current;
            current = current->nextRunner;

            cutsceneRunnerCancel(toRemove);

            if (previousCutscene) {
                previousCutscene->nextRunner = toRemove->nextRunner;
            } else {
                gRunningCutscenes = toRemove->nextRunner;
            }

            toRemove->nextRunner = gUnusedRunners;
            gUnusedRunners = toRemove;            
        } else {
            previousCutscene = current;
            current = current->nextRunner;
        }
    }
}


int cutsceneIsRunning(struct Cutscene* cutscene) {
    struct CutsceneRunner* current = gRunningCutscenes;

    while (current) {
        if (current->currentCutscene == cutscene) {
            return 1;
        }

        current = current->nextRunner;
    }


    return 0;
}

float cutsceneSoundQueueTime(int channel) {
    float result = 0.0f;

    if (soundPlayerIsPlaying(gCutsceneCurrentSound[channel])) {
        result += soundPlayerTimeLeft(gCutsceneCurrentSound[channel]);
    }

    struct QueuedSound* curr = gCutsceneSoundQueues[channel];

    while (curr) {
        result += soundClipDuration(curr->soundId, gCutsceneChannelPitch[channel]);
        curr = curr->next;
    }

    return result;
}

void cutscenesUpdateSounds() {
    for (int i = 0; i < CH_COUNT; ++i) {
        int soundType = SoundTypeNone;
        int subtitleType = SubtitleTypeNone; 
        if (i == CH_GLADOS){
            soundType = SoundTypeAll;
            subtitleType = SubtitleTypeCloseCaption; 
        }else if (i == CH_MUSIC){
            soundType = SoundTypeMusic;
        }else if (i == CH_AMBIENT){
            soundType = SoundTypeAll;
        }

        if (!soundPlayerIsPlaying(gCutsceneCurrentSound[i])) {
            if (gCutsceneSoundQueues[i]) {
                struct QueuedSound* curr = gCutsceneSoundQueues[i];

                gCutsceneCurrentSound[i] = soundPlayerPlay(curr->soundId, curr->volume, gCutsceneChannelPitch[i], NULL, NULL, soundType);
                gCutsceneCurrentSoundId[i] = curr->soundId;
                gCutsceneCurrentSubtitleId[i] = curr->subtitleId;
                gCutsceneCurrentVolume[i] = curr->volume;
                if (curr->subtitleId != SubtitleKeyNone){
                    hudShowSubtitle(&gScene.hud, curr->subtitleId, subtitleType);
                }

                gCutsceneSoundQueues[i] = curr->next;

                curr->next = gCutsceneNextFreeSound;
                gCutsceneNextFreeSound = curr;
            } else {
                if (gCutsceneCurrentSound[i] != SOUND_ID_NONE && i == CH_GLADOS) {
                    soundPlayerPlay(soundsIntercom[1], 1.0f, gCutsceneChannelPitch[i], NULL, NULL, soundType);
                    hudResolveSubtitle(&gScene.hud);
                }

                gCutsceneCurrentSound[i] = SOUND_ID_NONE;
                gCutsceneCurrentSoundId[i] = SOUND_ID_NONE;
                gCutsceneCurrentSubtitleId[i] = SubtitleKeyNone;
                gCutsceneCurrentVolume[i] = 0.0f;
            }
        }
    }
}

void cutscenesUpdate() {
    struct CutsceneRunner* previousCutscene = NULL;
    struct CutsceneRunner* current = gRunningCutscenes;

    cutscenesUpdateSounds();

    while (current) {
        if (cutsceneRunnerIsRunning(current)) {
            cutsceneRunnerUpdate(current);
            previousCutscene = current;
            current = current->nextRunner;
        } else {
            struct CutsceneRunner* toRemove = current;
            current = current->nextRunner;

            if (previousCutscene) {
                previousCutscene->nextRunner = toRemove->nextRunner;
            } else {
                gRunningCutscenes = toRemove->nextRunner;
            }

            toRemove->nextRunner = gUnusedRunners;
            gUnusedRunners = toRemove;            
        } 
    }
}

float cutsceneRunnerEstimateTimeLeft(struct CutsceneRunner* cutsceneRunner) {
    float result = 0.0f;

    for (int i = cutsceneRunner->currentStep; i < cutsceneRunner->currentCutscene->stepCount; ++i) {
        result += cutsceneStepEstimateTime(&cutsceneRunner->currentCutscene->steps[i], i == cutsceneRunner->currentStep ? &cutsceneRunner->state : NULL);
    }

    return result;
}

float cutsceneEstimateTimeLeft(struct Cutscene* cutscene) {
    struct CutsceneRunner* current = gRunningCutscenes;

    while (current) {
        if (current->currentCutscene == cutscene) {
            return cutsceneRunnerEstimateTimeLeft(current);
        }

        current = current->nextRunner;
    }


    return 0.0f;
}

int cutsceneTrigger(int cutsceneIndex, int triggerIndex) {
    u64 cutsceneMask = 1LL << triggerIndex;

    if (cutsceneIndex != -1 && !(gTriggeredCutscenes & cutsceneMask)) {
        cutsceneStart(&gCurrentLevel->cutscenes[cutsceneIndex]);
        // prevent the trigger from happening again
        gTriggeredCutscenes |= cutsceneMask;

        return 1;
    }

    return (gTriggeredCutscenes & cutsceneMask) != 0;
}

void cutsceneSerialize(struct CutsceneRunner* runner, struct CutsceneSerialized* result) {
    result->cutsceneIndex = runner->currentCutscene - gCurrentLevel->cutscenes;
    result->currentStep = runner->currentStep;

    struct CutsceneStep* step = &runner->currentCutscene->steps[runner->currentStep];

    switch (step->type) {
        case CutsceneStepTypePlaySound:
        case CutsceneStepTypeStartSound:
            result->state.playSound.soundId = SOUND_ID_NONE;
            break;
        default:
            result->state = runner->state;
            break;
    }
}

void cutsceneStartSerialized(struct CutsceneSerialized* serialized) {
    struct CutsceneRunner* runner = cutsceneRunnerNew();
    runner->nextRunner = gRunningCutscenes;
    gRunningCutscenes = runner;

    runner->currentCutscene = &gCurrentLevel->cutscenes[serialized->cutsceneIndex];
    runner->currentStep = serialized->currentStep;
    runner->state = serialized->state;
}

int cutsceneGetCount() {
    struct CutsceneRunner* curr = gRunningCutscenes;
    int result = 0;

    while (curr) {
        curr = curr->nextRunner;
        ++result;
    }

    return result;
}

void cutsceneSerializeWrite(struct Serializer* serializer, SerializeAction action) {
    short cutsceneCount = (short)cutsceneGetCount();
    action(serializer, &cutsceneCount, sizeof(short));

    struct CutsceneRunner* currCutscene = gRunningCutscenes;

    while (currCutscene) {
        struct CutsceneSerialized cutscene;
        cutsceneSerialize(currCutscene, &cutscene);
        action(serializer, &cutscene, sizeof(struct CutsceneSerialized));

        currCutscene = currCutscene->nextRunner;
    }

    action(serializer, &gTriggeredCutscenes, sizeof(gTriggeredCutscenes));

    for (int i = 0; i < CH_COUNT; ++i) {
        s16 curr = gCutsceneCurrentSound[i];
        action(serializer, &curr, sizeof(curr));

        u16 currId = gCutsceneCurrentSoundId[i];
        action(serializer, &currId, sizeof(currId));

        u16 subtitleId = gCutsceneCurrentSubtitleId[i];
        action(serializer, &subtitleId, sizeof(subtitleId));

        u8 volume = (u8)(clampf(gCutsceneCurrentVolume[i], 0.0f, 1.0f) * 255.0f);
        action(serializer, &volume, sizeof(volume));
    }

    for (int i = 0; i < CH_COUNT; ++i) {
        struct QueuedSound* curr = gCutsceneSoundQueues[i];

        while (curr) {
            action(serializer, &curr->soundId, sizeof(u16));
            action(serializer, &curr->subtitleId, sizeof(s16));
            u8 volume = (u8)(clampf(curr->volume, 0.0f, 1.0f) * 255.0f);
            action(serializer, &volume, sizeof(volume));
            curr = curr->next;
        }

        s16 noSound = SOUND_ID_NONE;
        action(serializer, &noSound, sizeof(noSound));
    }
}

void cutsceneSerializeRead(struct Serializer* serializer) {
    short cutsceneCount;
    serializeRead(serializer, &cutsceneCount, sizeof(short));

    cutsceneRunnerReset();

    for (int i = 0; i < cutsceneCount; ++i) {
        struct CutsceneSerialized cutscene;
        serializeRead(serializer, &cutscene, sizeof(struct CutsceneSerialized));
        cutsceneStartSerialized(&cutscene);
    }

    serializeRead(serializer, &gTriggeredCutscenes, sizeof (gTriggeredCutscenes));

    for (int i = 0; i < CH_COUNT; ++i) {
        s16 curr;
        serializeRead(serializer, &curr, sizeof(curr));
        gCutsceneCurrentSound[i] = curr;

        u16 currId;
        serializeRead(serializer, &currId, sizeof(currId));
        gCutsceneCurrentSoundId[i] = currId;

        u16 subtitleId;
        serializeRead(serializer, &subtitleId, sizeof(subtitleId));
        gCutsceneCurrentSubtitleId[i] = subtitleId;

        u8 volume;
        serializeRead(serializer, &volume, sizeof(volume));
        gCutsceneCurrentVolume[i] = volume * (1.0f / 255.0f);
        if (curr != SOUND_ID_NONE){
            cutsceneQueueSound(gCutsceneCurrentSoundId[i], gCutsceneCurrentVolume[i], i, gCutsceneCurrentSubtitleId[i]);
        }
    }

    for (int i = 0; i < CH_COUNT; ++i) {
        s16 nextId;
        serializeRead(serializer, &nextId, sizeof(nextId));
        

        while (nextId != SOUND_ID_NONE) {
            s16 nextSubtitleId;
            serializeRead(serializer, &nextSubtitleId, sizeof(nextSubtitleId));
            u8 volume;
            serializeRead(serializer, &volume, sizeof(volume));
            cutsceneQueueSound(nextId, volume * (1.0f / 255.0f), i, nextSubtitleId);
            serializeRead(serializer, &nextId, sizeof(nextId));
        }
    }
}

void cutsceneQueueSoundInChannel(int soundId, float volume, int channel, int subtitleId) {
    soundId = mapLocaleSound(soundId);
    
    if (!gCutsceneSoundQueues[channel] && !soundPlayerIsPlaying(gCutsceneCurrentSound[channel]) && channel == CH_GLADOS) {
        cutsceneQueueSound(soundsIntercom[0], volume, channel, subtitleId);
    }

    cutsceneQueueSound(soundId, volume, channel, subtitleId);
}

int cutsceneIsSoundQueued(){
    int soundQueued = 0;
    for (int i = 0; i < CH_COUNT; ++i) {
        if((gCutsceneSoundQueues[i] != NULL || gCutsceneCurrentSound[i] != SOUND_ID_NONE) && (i == CH_GLADOS)){
            soundQueued = 1;
            break;
        }
    }
    return soundQueued;
}
