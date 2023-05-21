#include "cutscene_runner.h"
#include "../audio/soundplayer.h"
#include "../util/time.h"
#include "../scene/scene.h"
#include "../scene/signals.h"
#include "../levels/levels.h"
#include "../util/memory.h"
#include "../savefile/checkpoint.h"

struct CutsceneRunner* gRunningCutscenes;
struct CutsceneRunner* gUnusedRunners;
u64 gTriggeredCutscenes;

#define MAX_QUEUE_LENGTH    25

struct QueuedSound {
    struct QueuedSound* next;
    u16 soundId;
    float volume;
};

struct QueuedSound gCutsceneSoundNodes[MAX_QUEUE_LENGTH];
struct QueuedSound* gCutsceneNextFreeSound;

struct QueuedSound* gCutsceneSoundQueues[CH_COUNT];
ALSndId gCutsceneCurrentSound[CH_COUNT];

float gCutsceneChannelPitch[CH_COUNT] = {
    [CH_GLADOS] = 0.5f,
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

void cutsceneQueueSound(int soundId, float volume, int channel) {
    struct QueuedSound* next = gCutsceneNextFreeSound;

    if (!next) {
        return;
    }

    gCutsceneNextFreeSound = gCutsceneNextFreeSound->next;

    next->next = NULL;
    next->soundId = soundId;
    next->volume = volume;

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

void cutsceneRunnerStartStep(struct CutsceneRunner* runner) {
    struct CutsceneStep* step = &runner->currentCutscene->steps[runner->currentStep];
    struct CutsceneStep* prev_step;
    struct CutsceneStep* next_step;
    enum CutsceneStepType prev_type = -1;
    enum CutsceneStepType next_type = -1;
    if (runner->currentStep != 0){
        prev_step = &runner->currentCutscene->steps[(runner->currentStep)-1];
        prev_type = prev_step->type;
    }
    if (runner->currentStep < runner->currentCutscene->stepCount){
        next_step = &runner->currentCutscene->steps[(runner->currentStep)+1];
        next_type = next_step->type;
    }
    
    switch (step->type) {
        case CutsceneStepTypePlaySound:
        case CutsceneStepTypeStartSound:
            runner->state.playSound.soundId = soundPlayerPlay(
                step->playSound.soundId,
                step->playSound.volume * (1.0f / 255.0f),
                step->playSound.pitch * (1.0f / 64.0f),
                NULL,
                NULL
            );
            break;
        case CutsceneStepTypeQueueSound:
        {
            if ((prev_type == -1) || !(prev_type == CutsceneStepTypeQueueSound)){
                cutsceneQueueSound(soundsIntercom[0], step->queueSound.volume * (1.0f / 255.0f), step->queueSound.channel);
            } 
            cutsceneQueueSound(step->queueSound.soundId, step->queueSound.volume * (1.0f / 255.0f), step->queueSound.channel);
            if ((next_type == -1) || !(next_type == CutsceneStepTypeQueueSound)){
                cutsceneQueueSound(soundsIntercom[1], step->queueSound.volume * (1.0f / 255.0f), step->queueSound.channel);
            } 
            break;
        }
        case CutsceneStepTypeDelay:
            runner->state.delay = step->delay;
            break;
        case CutsceneStepTypeOpenPortal:
        {
            struct Location* location = &gCurrentLevel->locations[step->openPortal.locationIndex];
            struct Ray firingRay;
            struct Vector3 transformUp;
            firingRay.origin = location->transform.position;
            quatMultVector(&location->transform.rotation, &gForward, &firingRay.dir);
            quatMultVector(&location->transform.rotation, &gRight, &transformUp);
            vector3Negate(&transformUp, &transformUp);
            vector3AddScaled(&location->transform.position, &firingRay.dir, -0.1f, &firingRay.origin);
            sceneFirePortal(&gScene, &firingRay, &transformUp, step->openPortal.portalIndex, location->roomIndex, 0, 0);
            break;
        }
        case CutsceneStepTypeClosePortal:
        {
            sceneClosePortal(&gScene, step->closePortal.portalIndex);
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
        default:
    }
}

int cutsceneRunnerUpdateCurrentStep(struct CutsceneRunner* runner) {
    struct CutsceneStep* step = &runner->currentCutscene->steps[runner->currentStep];
    switch (step->type) {
        case CutsceneStepTypePlaySound:
            return !soundPlayerIsPlaying(runner->state.playSound.soundId);
        case CutsceneStepTypeWaitForChannel:
        {
            int result = !soundPlayerIsPlaying(gCutsceneCurrentSound[step->waitForChannel.channel]) && gCutsceneSoundQueues[step->waitForChannel.channel] == NULL;
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

void cutscenesUpdateSounds() {
    for (int i = 0; i < CH_COUNT; ++i) {
        if (!soundPlayerIsPlaying(gCutsceneCurrentSound[i])) {
            if (gCutsceneSoundQueues[i]) {
                struct QueuedSound* curr = gCutsceneSoundQueues[i];

                gCutsceneCurrentSound[i] = soundPlayerPlay(curr->soundId, curr->volume, gCutsceneChannelPitch[i], NULL, NULL);
                gCutsceneSoundQueues[i] = curr->next;

                curr->next = gCutsceneNextFreeSound;
                gCutsceneNextFreeSound = curr;
            } else {
                gCutsceneCurrentSound[i] = SOUND_ID_NONE;
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


void cutsceneCheckTriggers(struct Vector3* playerPos) {
    for (int i = 0; i < gCurrentLevel->triggerCount; ++i) {
        struct Trigger* trigger = &gCurrentLevel->triggers[i];
        u64 cutsceneMask = 1LL << i;
        if (box3DContainsPoint(&trigger->box, playerPos)) {
            if (trigger->signalIndex != -1) {
                signalsSend(trigger->signalIndex);
            }

            if (trigger->cutsceneIndex != -1 && !(gTriggeredCutscenes & cutsceneMask)) {
                cutsceneStart(&gCurrentLevel->cutscenes[trigger->cutsceneIndex]);
                // prevent the trigger from happening again
                gTriggeredCutscenes |= cutsceneMask;
            }
        }
    }
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
        struct QueuedSound* curr = gCutsceneSoundQueues[i];

        while (curr) {
            action(serializer, &curr->soundId, sizeof(u16));
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
        s16 nextId;
        serializeRead(serializer, &nextId, sizeof(nextId));

        while (nextId != SOUND_ID_NONE) {
            u8 volume;
            serializeRead(serializer, &volume, sizeof(volume));
            cutsceneQueueSound(nextId, volume * (1.0f / 255.0f), i);
            serializeRead(serializer, &nextId, sizeof(nextId));
        }
    }
}