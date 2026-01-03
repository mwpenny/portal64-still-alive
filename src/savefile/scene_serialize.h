#ifndef __SCENE_SERIALIZE_H__
#define __SCENE_SERIALIZE_H__

#include "serializer.h"
#include "scene/scene.h"

void sceneSerialize(struct Serializer* serializer, SerializeAction action, struct Scene* scene);
void sceneDeserialize(struct Serializer* serializer, struct Scene* scene);

#endif
