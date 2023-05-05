#ifndef __CUTSCENE_RUNNER_H__
#define __CUTSCENE_RUNNER_H__

#include <ultra64.h>
#include "level_definition.h"
#include "../audio/clips.h"
#include "../savefile/serializer.h"

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

void cutsceneCheckTriggers(struct Vector3* playerPos);

void cutsceneSerialize(struct CutsceneRunner* runner, struct CutsceneSerialized* result);
void cutsceneStartSerialized(struct CutsceneSerialized* serialized);

void cutsceneSerializeWrite(struct Serializer* serializer, SerializeAction action);
void cutsceneSerializeRead(struct Serializer* serializer);

#endif