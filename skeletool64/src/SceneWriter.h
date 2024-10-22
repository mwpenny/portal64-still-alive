#ifndef _SCENE_WRITER_H
#define _SCENE_WRITER_H

#include <assimp/mesh.h>
#include <assimp/scene.h>
#include <sstream>
#include <string>
#include <map>
#include "./materials/Material.h"
#include "./DisplayListSettings.h"
#include "CFileDefinition.h"

void generateMeshFromScene(const aiScene* scene, CFileDefinition& fileDefinition, DisplayListSettings& settings);
void generateMeshFromSceneToFile(const aiScene* scene, std::string filename, DisplayListSettings& settings);

#endif