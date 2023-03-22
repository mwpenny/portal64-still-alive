
#include "../scene/scene.h"

typedef void* Checkpoint;

struct PartialTransform {
    struct Vector3 position;
    struct Quaternion rotation;
};

void checkpointClear();
void checkpointSave(struct Scene* scene);
void checkpointLoadLast(struct Scene* scene);
int checkpointExists();