#ifndef _SCENE_LOADER_H
#define _SCENE_LOADER_H

#include <assimp/scene.h>
#include <string>

aiScene* loadScene(const std::string& filename, bool isLevel, int vertexCacheSize);

#endif