#include "MeshDefinitionGenerator.h"

#include "../RenderChunk.h"
#include "../MeshWriter.h"
#include "AnimationGenerator.h"
#include "../StringUtils.h"
#include "MaterialGenerator.h"

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

        std::string materialName = ExtendedMesh::GetMaterialName(scene->mMaterials[mesh->mMesh->mMaterialIndex], settings.mForceMaterialName);

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
                node,
                materialPtr
            ));
        }

        for (auto pairSegment = mesh->mBoneSpanningFaces.begin(); pairSegment != mesh->mBoneSpanningFaces.end(); ++pairSegment) {
            renderChunks.push_back(RenderChunk(pairSegment->first, mesh, node, materialPtr));
        }
    }

    BoneHierarchy& bones = fileDefinition.GetBoneHierarchy();

    int attachmentCount = 0;

    for (unsigned i = 0; i < bones.GetBoneCount(); ++i) {
        Bone* bone = bones.BoneByIndex(i);

        if (StartsWith(bone->GetName(), "attachment ")) {
            fileDefinition.AddMacro(fileDefinition.GetMacroName(std::string("ATTACHMENT_") + bone->GetName().substr(strlen("attachment "))), std::to_string(attachmentCount));
            renderChunks.push_back(RenderChunk(std::make_pair(bone, bone), attachmentCount, NULL));
            ++attachmentCount;
        }
    }

    fileDefinition.AddMacro(fileDefinition.GetMacroName("ATTACHMENT_COUNT"), std::to_string(attachmentCount));
}

void MeshDefinitionGenerator::PopulateBones(const aiScene* scene, CFileDefinition& fileDefinition) {
    auto animInfo = findNodesForWithAnimation(scene, mIncludedNodes, mSettings.mModelScale);

    if (!mSettings.mBonesAsVertexGroups) {
        fileDefinition.GetBoneHierarchy().PopulateWithAnimationNodeInfo(*animInfo, mSettings.mFixedPointScale, mSettings.mRotateModel);
    }
}

void MeshDefinitionGenerator::GenerateDefinitions(const aiScene* scene, CFileDefinition& fileDefinition) {
    GenerateDefinitionsWithResults(scene, fileDefinition);
}

MeshDefinitionResults MeshDefinitionGenerator::GenerateDefinitionsWithResults(const aiScene* scene, CFileDefinition& fileDefinition) {
    std::vector<RenderChunk> renderChunks;

    for (auto node = mIncludedNodes.begin(); node != mIncludedNodes.end(); ++node) {
        AppendRenderChunks(scene, *node, fileDefinition, mSettings, renderChunks);
    }

    MeshDefinitionResults result;

    result.modelName = generateMesh(scene, fileDefinition, renderChunks, mSettings, "_geo");
    result.materialMacro = MaterialGenerator::MaterialIndexMacroName(mSettings.mDefaultMaterialName);

    if (fileDefinition.GetBoneHierarchy().HasData() && !mSettings.mBonesAsVertexGroups) {
        generateAnimationForScene(scene, fileDefinition, mSettings);
    }
    
    return result;
}