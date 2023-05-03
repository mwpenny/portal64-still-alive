#include "checkpoint.h"

#include "../util/memory.h"
#include "../levels/cutscene_runner.h"
#include "../levels/levels.h"

char gHasCheckpoint = 0;
char gCheckpoint[MAX_CHECKPOINT_SIZE];

void* checkpointWrite(void* dst, int size, void* src) {
    memCopy(dst, src, size);
    return (char*)dst + size;
}

void* checkpointRead(void* src, int size, void* dst) {
    memCopy(dst, src, size);
    return (char*)src + size;
}

extern unsigned long long* gSignals;
extern unsigned long long* gDefaultSignals;
extern unsigned gSignalCount;

extern struct CutsceneRunner* gRunningCutscenes;

extern u64 gTriggeredCutscenes;

int checkpointCutsceneCount() {
    struct CutsceneRunner* curr = gRunningCutscenes;
    int result = 0;

    while (curr) {
        curr = curr->nextRunner;
        ++result;
    }

    return result;
}

int checkpointEstimateSize(struct Scene* scene) {
    int result = 0;

    int binCount = SIGNAL_BIN_COUNT(gSignalCount);
    result += sizeof(unsigned long long) * binCount * 2;

    result += sizeof(short);
    result += sizeof(struct CutsceneSerialized) * checkpointCutsceneCount();

    result += sizeof(struct PartialTransform);

    result += sizeof(gTriggeredCutscenes);

    return result;
}

void checkpointClear() {
    gHasCheckpoint = 0;
}

void checkpointUse(Checkpoint checkpoint) {
    memCopy(gCheckpoint, checkpoint, MAX_CHECKPOINT_SIZE);
    gHasCheckpoint = 1;
}

int checkpointExists() {
    if (gHasCheckpoint != 0){
        return 1;
    }
    return 0;
}

void checkpointSave(struct Scene* scene) {
    gHasCheckpoint = checkpointSaveInto(scene, gCheckpoint);
    gHasCheckpoint = 1;
}

void checkpointLoadLast(struct Scene* scene) {
    if (!gHasCheckpoint) {
        return;
    }

    checkpointLoadLastFrom(scene, gCheckpoint);
}

int checkpointSaveInto(struct Scene* scene, Checkpoint into) {
    int size = checkpointEstimateSize(scene);

    if (size > MAX_CHECKPOINT_SIZE) {
        return 0;
    }

    void* curr = into;

    int binCount = SIGNAL_BIN_COUNT(gSignalCount);
    curr = checkpointWrite(curr, sizeof(unsigned long long) * binCount, gSignals);
    curr = checkpointWrite(curr, sizeof(unsigned long long) * binCount, gDefaultSignals);

    short cutsceneCount = (short)checkpointCutsceneCount();
    curr = checkpointWrite(curr, sizeof(short), &cutsceneCount);

    struct CutsceneRunner* currCutscene = gRunningCutscenes;

    while (currCutscene) {
        struct CutsceneSerialized cutscene;
        cutsceneSerialize(currCutscene, &cutscene);
        curr = checkpointWrite(curr, sizeof(struct CutsceneSerialized), &cutscene);

        currCutscene = currCutscene->nextRunner;
    }

    curr = checkpointWrite(curr, sizeof(struct PartialTransform), &scene->player.body.transform);

    curr = checkpointWrite(curr, sizeof (gTriggeredCutscenes), &gTriggeredCutscenes);

    return 1;
}

void checkpointLoadLastFrom(struct Scene* scene, Checkpoint from) {
    void* curr = from;

    int binCount = SIGNAL_BIN_COUNT(gSignalCount);
    curr = checkpointRead(curr, sizeof(unsigned long long) * binCount, gSignals);
    curr = checkpointRead(curr, sizeof(unsigned long long) * binCount, gDefaultSignals);

    short cutsceneCount;
    curr = checkpointRead(curr, sizeof(short), &cutsceneCount);

    cutsceneRunnerReset();

    for (int i = 0; i < cutsceneCount; ++i) {
        struct CutsceneSerialized cutscene;
        curr = checkpointRead(curr, sizeof(struct CutsceneSerialized), &cutscene);
        cutsceneStartSerialized(&cutscene);
    }

    curr = checkpointRead(curr, sizeof(struct PartialTransform), &scene->player.body.transform);
    scene->player.body.velocity = gZeroVec;

    curr = checkpointRead(curr, sizeof (gTriggeredCutscenes), &gTriggeredCutscenes);
}