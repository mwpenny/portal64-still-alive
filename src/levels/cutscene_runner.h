#ifndef __CUTSCENE_RUNNER_H__
#define __CUTSCENE_RUNNER_H__

#include <ultra64.h>
#include "level_definition.h"

struct CutsceneRunner {
    struct Cutscene currentCutscene;
    u16 currentStep;

    union {
        struct {
            ALSndId soundId;
        } playSound;
        float delay;
    } state;
};

extern struct CutsceneRunner gCutsceneRunner;

void cutsceneRunnerRun(struct CutsceneRunner* runner, struct Cutscene* cutscene);
void cutsceneRunnerUpdate(struct CutsceneRunner* runner);

int cutsceneRunnerIsRunning(struct CutsceneRunner* runner);

#endif