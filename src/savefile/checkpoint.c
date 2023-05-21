#include "checkpoint.h"

#include "../util/memory.h"
#include "../levels/cutscene_runner.h"
#include "../levels/levels.h"
#include "./scene_serialize.h"
#include "serializer.h"
#include "./savefile.h"

char gHasCheckpoint = 0;
char __attribute__((aligned(8))) gCheckpoint[MAX_CHECKPOINT_SIZE];

void ckeckpointSerialize(struct Serializer* serializer, SerializeAction action, void* data) {
    struct Scene* scene = data;

    signalsSerializeRW(serializer, action);
    cutsceneSerializeWrite(serializer, action);
    sceneSerialize(serializer, action, scene);
}

void checkpointDeserialize(struct Serializer* serializer, void* data) {
    struct Scene* scene = data;

    signalsSerializeRW(serializer, serializeRead);
    cutsceneSerializeRead(serializer);
    sceneDeserialize(serializer, scene);
}

int checkpointEstimateSize(struct Scene* scene) {
    struct Serializer serializer = {NULL};
    ckeckpointSerialize(&serializer, serializeCount, scene);
    return (int)serializer.curr;
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
    savefileGrabScreenshot();
    gHasCheckpoint = checkpointSaveInto(scene, gCheckpoint);
    // slot 0 is the autosave slot
    savefileSaveGame(gCheckpoint, gScreenGrabBuffer, gCurrentLevelIndex, gCurrentTestSubject, 0);
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

    struct Serializer serializer = {into};
    ckeckpointSerialize(&serializer, serializeWrite, scene);

    return 1;
}

void checkpointLoadLastFrom(struct Scene* scene, Checkpoint from) {
    struct Serializer serializer = {from};
    checkpointDeserialize(&serializer, scene);
}