#ifndef __SAVEFILE_CHECKPOINT_H__
#define __SAVEFILE_CHECKPOINT_H__

#include "scene/scene.h"

#define MAX_CHECKPOINT_SIZE 2048

typedef void* Checkpoint;

int checkpointExists();
void checkpointClear();

int checkpointSave(struct Scene* scene, int slotIndex);
void checkpointQueueLoad(int slotIndex);
void checkpointLoadCurrent(struct Scene* scene);

#endif
