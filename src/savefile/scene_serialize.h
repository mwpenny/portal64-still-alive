#ifndef __SCENE_SERIALIZE_H__
#define __SCENE_SERIALIZE_H__

#include "../scene/scene.h"
#include "./serializer.h"

struct PartialTransform {
    struct Vector3 position;
    struct Quaternion rotation;
};

void sceneSerialize(struct Serializer* serializer, SerializeAction action, struct Scene* scene);
void sceneDeserialize(struct Serializer* serializer, struct Scene* scene);

#endif