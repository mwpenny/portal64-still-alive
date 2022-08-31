#include "cutscene_runner.h"
#include "../audio/soundplayer.h"
#include "../util/time.h"
#include "../scene/scene.h"
#include "../scene/signals.h"
#include "../levels/levels.h"
#include "../util/memory.h"

struct CutsceneRunner* gRunningCutscenes;
struct CutsceneRunner* gUnusedRunners;

#define MAX_QUEUE_LENGTH    16

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

ALSndId cutsceneRunnerPlaySound(struct CutsceneStep* step) {
    return soundPlayerPlay(
        step->playSound.soundId,
        step->playSound.volume * (1.0f / 255.0f),
        step->playSound.pitch * (1.0f / 64.0f),
        NULL
    );
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

void cutsceneRunnerStartStep(struct CutsceneRunner* runner) {
    struct CutsceneStep* step = &runner->currentCutscene->steps[runner->currentStep];
    switch (step->type) {
        case CutsceneStepTypePlaySound:
        case CutsceneStepTypeStartSound:
            runner->state.playSound.soundId = cutsceneRunnerPlaySound(step);
            break;
        case CutsceneStepTypeQueueSound:
            cutsceneQueueSound(step->queueSound.soundId, step->queueSound.volume * (1.0f / 255.0f), step->queueSound.channel);
            break;
        case CutsceneStepTypeDelay:
            runner->state.delay = step->delay;
            break;
        case CutsceneStepTypeOpenPortal:
        {
            struct Location* location = &gCurrentLevel->locations[step->openPortal.locationIndex];
            struct Ray firingRay;
            firingRay.origin = location->transform.position;
            quatMultVector(&location->transform.rotation, &gForward, &firingRay.dir);
            vector3AddScaled(&location->transform.position, &firingRay.dir, -0.1f, &firingRay.origin);
            sceneFirePortal(&gScene, &firingRay, &gUp, step->openPortal.portalIndex, location->roomIndex);
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
                gCurrentLevel->locations[step->teleportPlayer.toLocation].roomIndex
            );
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
            break;
        case CutsceneStepTypePointPedestal:
            for (unsigned i = 0; i < gScene.pedestalCount; ++i) {
                pedestalPointAt(&gScene.pedestals[i], &gCurrentLevel->locations[step->pointPedestal.atLocation].transform.position);
            }
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
    cutsceneRunnerRun(runner, cutscene);
    runner->nextRunner = gRunningCutscenes;
    gRunningCutscenes = runner;
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

                gCutsceneCurrentSound[i] = soundPlayerPlay(curr->soundId, curr->volume, gCutsceneChannelPitch[i], NULL);
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