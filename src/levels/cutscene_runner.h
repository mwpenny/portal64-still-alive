#ifndef __CUTSCENE_RUNNER_H__
#define __CUTSCENE_RUNNER_H__

#include <ultra64.h>
#include "level_definition.h"

struct CutsceneRunner {
    struct Cutscene* currentCutscene;
    u16 currentStep;

    union {
        struct {
            ALSndId soundId;
        } playSound;
        float delay;
    } state;

    struct CutsceneRunner* nextRunner;
};


void cutsceneStart(struct Cutscene* cutscene);
void cutsceneStop(struct Cutscene* cutscene);
int cutsceneIsRunning(struct Cutscene* cutscene);
void cutscenesUpdate();

#endif