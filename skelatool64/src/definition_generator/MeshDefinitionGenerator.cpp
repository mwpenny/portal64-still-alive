#include "MeshDefinitionGenerator.h"

#include "../RenderChunk.h"
#include "../MeshWriter.h"
#include "AnimationGenerator.h"

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

void MeshDefinitionGenerator::AppendRenderChunks(const aiScene* scene, aiNode* node, CFileDefinition& fileDefinition, const DisplayListSettings& settings, std::vector<RenderChunk>& renderChunks) {
    for (unsigned meshIndex = 0; meshIndex < node->mNumMeshes; ++meshIndex) {
        std::shared_ptr<ExtendedMesh> mesh = fileDefinition.GetExtendedMesh(scene->mMeshes[node->mMeshes[meshIndex]]);

        mesh = mesh->Transform(node->mTransformation);

        std::string materialName = ExtendedMesh::GetMaterialName(scene->mMaterials[mesh->mMesh->mMaterialIndex]);

        auto material = settings.mMaterials.find(materialName);

        Material* materialPtr = NULL;

        if (material != settings.mMaterials.end()) {
            materialPtr = material->second.get();
        } else {
            std::cout << "Could not find material with name " << materialName << std::endl;
        }

        double sTile;
        double tTile;

        if (extractMaterialAutoTileParameters(materialPtr, sTile, tTile)) {
            mesh->CubeProjectTex(
                settings.mModelScale / (double)sTile,
                settings.mModelScale / (double)tTile
            );
        }


        for (auto boneSegment = mesh->mFacesForBone.begin(); boneSegment != mesh->mFacesForBone.end(); ++boneSegment) {
            renderChunks.push_back(RenderChunk(
                std::make_pair(boneSegment->first, boneSegment->first),
                mesh,
                materialPtr
            ));
        }

        for (auto pairSegment = mesh->mBoneSpanningFaces.begin(); pairSegment != mesh->mBoneSpanningFaces.end(); ++pairSegment) {
            renderChunks.push_back(RenderChunk(pairSegment->first, mesh, materialPtr));
        }
    }
}

void MeshDefinitionGenerator::GenerateDefinitions(const aiScene* scene, CFileDefinition& fileDefinition) {
    std::vector<RenderChunk> renderChunks;

    auto animInfo = findNodesForWithAnimation(scene, mIncludedNodes, mSettings.CreateCollisionTransform());
    fileDefinition.GetBoneHierarchy().PopulateWithAnimationNodeInfo(*animInfo, mSettings.mFixedPointScale);

    for (auto node = mIncludedNodes.begin(); node != mIncludedNodes.end(); ++node) {
        AppendRenderChunks(scene, *node, fileDefinition, mSettings, renderChunks);
    }

    generateMesh(scene, fileDefinition, renderChunks, mSettings, "_geo");

    if (fileDefinition.GetBoneHierarchy().HasData()) {
        generateAnimationForScene(scene, fileDefinition, mSettings);
    }
}