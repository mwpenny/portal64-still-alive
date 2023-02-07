
#include "scene.h"

typedef void* Checkpoint;

struct PartialTransform {
    struct Vector3 position;
    struct Quaternion rotation;
};

Checkpoint checkpointNew(struct Scene* scene);
void checkpointLoad(struct Scene* scene, Checkpoint checkpoint);
void checkpointDelete(Checkpoint checkpoint);