#include "checkpoint.h"

#include "../util/memory.h"
#include "../levels/cutscene_runner.h"

void* checkpointWrite(void* dst, int size, void* src) {
    memCopy(dst, src, size);
    return (char*)dst + size;
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

    result += sizeof(struct CutsceneSerialized) * checkpointCutsceneCount();

    result += sizeof(gTriggeredCutscenes);

    return result;
}

Checkpoint checkpointNew(struct Scene* scene) {
    int size = checkpointEstimateSize(scene);

    void* result = malloc(size);

    void* curr = result;

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
    }

    curr = checkpointWrite(curr, sizeof(struct PartialTransform), &scene->player.body.transform);

    curr = checkpointWrite(curr, sizeof (gTriggeredCutscenes), &gTriggeredCutscenes);

    return result;
}

void checkpointLoad(struct Scene* scene, Checkpoint checkpoint) {

}

void checkpointDelete(Checkpoint checkpoint) {
    free(checkpoint);
}