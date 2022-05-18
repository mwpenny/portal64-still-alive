#include "SceneLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include "SceneModification.h"
#include <iostream>

aiScene* loadScene(const std::string& filename, bool isLevel, int vertexCacheSize) {
    Assimp::Importer importer;

    importer.SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, 1);

    unsigned int pFlags = aiProcess_JoinIdenticalVertices |
        aiProcess_Triangulate |
        aiProcess_LimitBoneWeights |
        aiProcess_OptimizeMeshes;

    if (!isLevel) {
        importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);
        pFlags |= aiProcess_OptimizeGraph | aiProcess_SortByPType;
    }

    const aiScene* scene = importer.ReadFile(filename, pFlags);

    if (scene == nullptr) {
        std::cerr << "Error loading input file: " << importer.GetErrorString() << std::endl;
        return 0;
    }

    if (!isLevel) {
        splitSceneByBones(const_cast<aiScene*>(scene));
    }

    importer.SetPropertyInteger(AI_CONFIG_PP_ICL_PTCACHE_SIZE, vertexCacheSize);
    importer.ApplyPostProcessing(aiProcess_ImproveCacheLocality);

    return importer.GetOrphanedScene();
}