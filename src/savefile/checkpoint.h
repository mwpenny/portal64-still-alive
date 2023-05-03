#ifndef __SAVEFILE_CHECKPOINT_H__
#define __SAVEFILE_CHECKPOINT_H__

#include "../scene/scene.h"

typedef void* Checkpoint;

#define MAX_CHECKPOINT_SIZE 2048

struct PartialTransform {
    struct Vector3 position;
    struct Quaternion rotation;
};

void checkpointClear();
void checkpointSave(struct Scene* scene);
void checkpointLoadLast(struct Scene* scene);

int checkpointSaveInto(struct Scene* scene, Checkpoint into);
void checkpointLoadLastFrom(struct Scene* scene, Checkpoint from);

int checkpointExists();

#endif