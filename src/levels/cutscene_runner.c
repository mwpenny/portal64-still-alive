#include "cutscene_runner.h"
#include "../audio/soundplayer.h"
#include "../util/time.h"
#include "../scene/scene.h"
#include "../scene/signals.h"
#include "../levels/levels.h"
#include "../util/memory.h"

struct CutsceneRunner* gRunningCutscenes;
struct CutsceneRunner* gUnusedRunners;

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
            soundPlayerStop(runner->state.playSound.soundId);
            break;
        default:
    }

    runner->currentStep = 0;
    runner->currentCutscene = NULL;
}

void cutsceneRunnerStartStep(struct CutsceneRunner* runner) {
    struct CutsceneStep* step = &runner->currentCutscene->steps[runner->currentStep];
    switch (step->type) {
        case CutsceneStepTypePlaySound:
        case CutsceneStepTypeStartSound:
            runner->state.playSound.soundId = soundPlayerPlay(
                step->playSound.soundId,
                step->playSound.volume * (1.0f / 255.0f),
                step->playSound.pitch * (1.0f / 64.0f),
                NULL
            );
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
        case CutsceneStepTypeGoto:
            runner->currentStep += step->gotoStep.relativeInstructionIndex;
            break;
        case CutsceneStepTypeStartCutscene:
            cutsceneStart(&gCurrentLevel->cutscenes[step->cutscene.cutsceneIndex]);
            break;
        case CutsceneStepTypeStopCutscene:
            cutsceneStop(&gCurrentLevel->cutscenes[step->cutscene.cutsceneIndex]);
            break;
        default:
    }
}

int cutsceneRunnerUpdateCurrentStep(struct CutsceneRunner* runner) {
    struct CutsceneStep* step = &runner->currentCutscene->steps[runner->currentStep];
    switch (step->type) {
        case CutsceneStepTypePlaySound:
            return !soundPlayerIsPlaying(runner->state.playSound.soundId);
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

void cutscenesUpdate() {
    struct CutsceneRunner* previousCutscene = NULL;
    struct CutsceneRunner* current = gRunningCutscenes;

    while (current) {
        cutsceneRunnerUpdate(current);

        if (!cutsceneRunnerIsRunning(current)) {
            struct CutsceneRunner* toRemove = current;
            current = current->nextRunner;

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