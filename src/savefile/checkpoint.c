#include "checkpoint.h"

#include "levels/cutscene_runner.h"
#include "levels/levels.h"
#include "savefile.h"
#include "scene_serialize.h"
#include "serializer.h"
#include "util/memory.h"

static int sCheckpointCurrentSlot = SAVEFILE_NO_SLOT;

static void checkpointSerialize(struct Serializer* serializer, SerializeAction action, struct Scene* scene) {
    signalsSerializeRW(serializer, action);
    cutsceneSerializeWrite(serializer, action);
    sceneSerialize(serializer, action, scene);
}

static void checkpointDeserialize(struct Serializer* serializer, struct Scene* scene) {
    signalsSerializeRW(serializer, serializeRead);
    cutsceneSerializeRead(serializer);
    sceneDeserialize(serializer, scene);
}

static int checkpointCalculateSize(struct Scene* scene) {
    struct Serializer serializer = { NULL };
    checkpointSerialize(&serializer, serializeCount, scene);
    return (int)serializer.curr;
}

static int checkpointSaveInto(struct Scene* scene, Checkpoint into) {
    int size = checkpointCalculateSize(scene);

    if (size > MAX_CHECKPOINT_SIZE) {
        return 0;
    }

    struct Serializer serializer = { into };
    checkpointSerialize(&serializer, serializeWrite, scene);

    return 1;
}

static void checkpointLoadFrom(struct Scene* scene, Checkpoint from) {
    struct Serializer serializer = { from };
    checkpointDeserialize(&serializer, scene);
}

static int checkpointSlotIsValid(int slotIndex) {
    if (slotIndex == SAVEFILE_NO_SLOT || savefileSlotIsFree(slotIndex)) {
        return 0;
    }

    struct SaveSlotInfo info;
    savefileGetSlotInfo(slotIndex, &info);

    return getLevelIndexFromChamberIndex(info.testChamberNumber) == gCurrentLevelIndex &&
        info.testSubjectNumber == gCurrentTestSubject;
}

static void checkpointUpdateCurrentSlot() {
    // The player can delete saves, so look for the most relevant existing save
    // Note that deleting saves only reorders in menus, not in memory

    if (sCheckpointCurrentSlot == SAVEFILE_NO_SLOT) {
        // Not trying to load a checkpoint
        return;
    }

    // Last slot used for save/load
    if (checkpointSlotIsValid(sCheckpointCurrentSlot)) {
        return;
    }

    // Most recent save from this run
    sCheckpointCurrentSlot = savefileLatestSubjectSlot(gCurrentTestSubject, 1 /* includeAuto */);
    if (checkpointSlotIsValid(sCheckpointCurrentSlot)) {
        return;
    }

    sCheckpointCurrentSlot = SAVEFILE_NO_SLOT;
}

void checkpointClear() {
    sCheckpointCurrentSlot = SAVEFILE_NO_SLOT;
}

int checkpointExists() {
    checkpointUpdateCurrentSlot();

    return sCheckpointCurrentSlot != SAVEFILE_NO_SLOT;
}

int checkpointSave(struct Scene* scene, int slotIndex) {
    Checkpoint* save = stackMalloc(MAX_CHECKPOINT_SIZE);
    int success = checkpointSaveInto(scene, save);

    if (success) {
        savefileSaveSlot(
            slotIndex,
            getChamberIndexFromLevelIndex(gCurrentLevelIndex, scene->player.body.currentRoom),
            gCurrentTestSubject,
            save
        );

        sCheckpointCurrentSlot = slotIndex;
    }

    stackMallocFree(save);

    return success;
}

void checkpointQueueLoad(int slotIndex) {
    struct SaveSlotInfo info;
    savefileGetSlotInfo(slotIndex, &info);

    sCheckpointCurrentSlot = slotIndex;
    gCurrentTestSubject = info.testSubjectNumber;
    levelQueueLoad(getLevelIndexFromChamberIndex(info.testChamberNumber), NULL, NULL, 1 /* useCheckpoint */);
}

void checkpointLoadCurrent(struct Scene* scene) {
    if (sCheckpointCurrentSlot == SAVEFILE_NO_SLOT) {
        return;
    }

    Checkpoint* save = stackMalloc(MAX_CHECKPOINT_SIZE);
    if (savefileLoadSlot(sCheckpointCurrentSlot, save)) {
        checkpointLoadFrom(scene, save);
    }
    stackMallocFree(save);
}
