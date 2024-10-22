#ifndef _SCENE_MODIFICAITON_H
#define _SCENE_MODIFICAITON_H

#include <assimp/mesh.h>
#include <assimp/BaseImporter.h>
#include <vector>

// Caller is responsible for freeing memory
aiMesh* subMesh(aiMesh* mesh, std::vector<aiFace*> faces, std::string name);

void splitSceneByBones(aiScene* targetScene);

#endif