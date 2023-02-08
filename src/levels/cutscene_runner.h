#ifndef __CUTSCENE_RUNNER_H__
#define __CUTSCENE_RUNNER_H__

#include <ultra64.h>
#include "level_definition.h"

union CutsceneStepState {
    struct {
        ALSndId soundId;
    } playSound;
    float delay;
}; 

struct CutsceneSerialized {
    u16 cutsceneIndex;
    u16 currentStep;
    union CutsceneStepState state;
};

struct CutsceneRunner {
    struct Cutscene* currentCutscene;
    u16 currentStep;
    union CutsceneStepState state;

    struct CutsceneRunner* nextRunner;
};

void cutsceneRunnerReset();
void cutsceneStart(struct Cutscene* cutscene);
void cutsceneStop(struct Cutscene* cutscene);
int cutsceneIsRunning(struct Cutscene* cutscene);
void cutscenesUpdate();

void cutsceneSerialize(struct CutsceneRunner* runner, struct CutsceneSerialized* result);
void cutsceneStartSerialized(struct CutsceneSerialized* serialized);

#endif