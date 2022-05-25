#include "cutscene_runner.h"
#include "../audio/soundplayer.h"
#include "../util/time.h"
#include "../scene/scene.h"
#include "../levels/levels.h"

struct CutsceneRunner gCutsceneRunner;

void cutsceneRunnerStartStep(struct CutsceneRunner* runner) {
    struct CutsceneStep* step = &runner->currentCutscene.steps[runner->currentStep];
    switch (step->type) {
        case CutsceneStepTypePlaySound:
        case CutsceneStepTypeStartSound:
            runner->state.playSound.soundId = soundPlayerPlay(
                step->playSound.soundId,
                step->playSound.volume * (1.0f / 255.0f),
                step->playSound.pitch * (1.0f / 64.0f)
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
        default:
    }
}

int cutsceneRunnerUpdateCurrentStep(struct CutsceneRunner* runner) {
    struct CutsceneStep* step = &runner->currentCutscene.steps[runner->currentStep];
    switch (step->type) {
        case CutsceneStepTypePlaySound:
            return !soundPlayerIsPlaying(runner->state.playSound.soundId);
        case CutsceneStepTypeDelay:
            runner->state.delay -= FIXED_DELTA_TIME;
            return runner->state.delay <= 0.0f;
        default:
            return 1;
    }
}

void cutsceneRunnerRun(struct CutsceneRunner* runner, struct Cutscene* cutscene) {
    runner->currentCutscene = *cutscene;
    runner->currentStep = 0;
    cutsceneRunnerStartStep(runner);
}

int cutsceneRunnerIsRunning(struct CutsceneRunner* runner) {
    return runner->currentStep < runner->currentCutscene.stepCount;
}

void cutsceneRunnerUpdate(struct CutsceneRunner* runner) {
    while (cutsceneRunnerIsRunning(runner) && cutsceneRunnerUpdateCurrentStep(runner)) {
        runner->currentStep++;

        if (cutsceneRunnerIsRunning(runner)) {
            cutsceneRunnerStartStep(runner);
        }
    }
}