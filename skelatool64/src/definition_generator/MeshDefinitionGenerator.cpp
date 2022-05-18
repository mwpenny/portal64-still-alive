#include "MeshDefinitionGenerator.h"

#include "../RenderChunk.h"
#include "../MeshWriter.h"

bool extractMaterialAutoTileParameters(Material* material, double& sTile, double& tTile) {
    if (!material) {
        return false;
    }

    auto sTileEntry = material->mProperties.find("tileSizeS");
    auto tTileEntry = material->mProperties.find("tileSizeT");

    if (sTileEntry != material->mProperties.end()) {
        sTile = std::atof(sTileEntry->second.c_str());
        tTile = tTileEntry == material->mProperties.end() ? sTile : std::atof(tTileEntry->second.c_str());
        return true;
    }

    return false;
}

MeshDefinitionGenerator::MeshDefinitionGenerator(const DisplayListSettings& settings) :
    DefinitionGenerator(),
    mSettings(settings) {

    }

bool MeshDefinitionGenerator::ShouldIncludeNode(aiNode* node) {
    return node->mName.C_Str()[0] != '@' && node->mNumMeshes > 0;
}

void MeshDefinitionGenerator::AppendRenderChunks(const aiScene* scene, aiNode* node, CFileDefinition& fileDefinition, DisplayListSettings& settings, std::vector<RenderChunk>& renderChunks) {
    for (unsigned meshIndex = 0; meshIndex < node->mNumMeshes; ++meshIndex) {
        std::shared_ptr<ExtendedMesh> mesh = fileDefinition.GetExtendedMesh(scene->mMeshes[node->mMeshes[meshIndex]]);

        mesh = mesh->Transform(node->mTransformation);

        std::string materialName = ExtendedMesh::GetMaterialName(scene->mMaterials[mesh->mMesh->mMaterialIndex]);

        auto material = settings.mMaterials.find(materialName);

        Material* materialPtr = NULL;

        if (material != settings.mMaterials.end()) {
            materialPtr = material->second.get();
        }

        double sTile;
        double tTile;

        if (extractMaterialAutoTileParameters(materialPtr, sTile, tTile)) {
            mesh->CubeProjectTex(
                settings.mCollisionScale / (double)sTile,
                settings.mCollisionScale / (double)tTile
            );
        }

        renderChunks.push_back(RenderChunk(
            std::pair<Bone*, Bone*>(NULL, NULL),
            mesh,
            materialPtr
        ));
    }
}

void MeshDefinitionGenerator::GenerateDefinitions(const aiScene* scene, CFileDefinition& fileDefinition) {
    std::vector<RenderChunk> renderChunks;

    for (auto node = mIncludedNodes.begin(); node != mIncludedNodes.end(); ++node) {
        AppendRenderChunks(scene, *node, fileDefinition, mSettings, renderChunks);
    }

    generateMesh(scene, fileDefinition, renderChunks, mSettings, "_geo");
}