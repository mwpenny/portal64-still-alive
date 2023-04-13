#include "MeshDefinitionGenerator.h"

#include "../RenderChunk.h"
#include "../MeshWriter.h"
#include "AnimationGenerator.h"
#include "../StringUtils.h"
#include "MaterialGenerator.h"
#include "../RenderChunkOrder.h"

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

double extractNumberValue(const std::string& nodeName, const std::string& label, double def) {
    std::size_t Attrat = nodeName.find(" " + label + " ");

    if (Attrat == std::string::npos) {
        return def;
    }

    std::size_t endAttrat = Attrat + label.length() + 2;
    std::size_t spacePos = nodeName.find(" ", endAttrat);

    if (spacePos == std::string::npos) {
        spacePos = nodeName.size();
    }

    std::string resultAsString = nodeName.substr(endAttrat, spacePos - endAttrat);

    double result = atof(resultAsString.c_str());

    if (result == 0.0) {
        return def;
    }

    return result;
}

aiMatrix4x4 buildTransformForRenderChunk(aiNode* node) {
    aiMatrix4x4 result;

    while (node) {
        result = node->mTransformation * result;
        node = node->mParent;
    }

    return result;
}

void MeshDefinitionGenerator::AppendRenderChunks(const aiScene* scene, aiNode* node, CFileDefinition& fileDefinition, const DisplayListSettings& settings, std::vector<RenderChunk>& renderChunks) {
    for (unsigned meshIndex = 0; meshIndex < node->mNumMeshes; ++meshIndex) {
        std::shared_ptr<ExtendedMesh> mesh = fileDefinition.GetExtendedMesh(scene->mMeshes[node->mMeshes[meshIndex]]);

        mesh = mesh->Transform(settings.CreateCollisionTransform() * buildTransformForRenderChunk(node));

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
            std::string nodeName = node->mName.C_Str();
            double uvScale = extractNumberValue(nodeName, "uvscale", 1.0f);
            mesh->CubeProjectTex(
                uvScale / (double)sTile,
                uvScale / (double)tTile,
                aiQuaternion(aiVector3D(1.0f, 0.0f, 0.0f), extractNumberValue(nodeName, "uvrotx", 0.0f) * (M_PI / 180.0f)) *
                aiQuaternion(aiVector3D(0.0f, 1.0f, 0.0f), extractNumberValue(nodeName, "uvroty", 0.0f) * (M_PI / 180.0f)) *
                aiQuaternion(aiVector3D(0.0f, 0.0f, 1.0f), extractNumberValue(nodeName, "uvrotz", 0.0f) * (M_PI / 180.0f)),
                aiVector3D(
                    extractNumberValue(nodeName, "uvtransx", 0.0f), 
                    extractNumberValue(nodeName, "uvtransy", 0.0f), 
                    extractNumberValue(nodeName, "uvtransz", 0.0f)
                )
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
            renderChunks.push_back(RenderChunk(std::make_pair(bone, bone), attachmentCount, NULL));
            ++attachmentCount;
        }
    }
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

    orderRenderChunks(renderChunks, mSettings);

    MeshDefinitionResults result;

    result.modelName = generateMesh(scene, fileDefinition, renderChunks, mSettings, "_geo");
    result.materialMacro = MaterialGenerator::MaterialIndexMacroName(mSettings.mDefaultMaterialName);

    if (fileDefinition.GetBoneHierarchy().HasData() && !mSettings.mBonesAsVertexGroups) {
        AnimationResults animationResults = generateAnimationForScene(scene, fileDefinition, mSettings);

        std::unique_ptr<StructureDataChunk> armatureDef(new StructureDataChunk());

        fileDefinition.AddHeader("\"sk64/skelatool_armature.h\"");

        armatureDef->AddPrimitive(result.modelName);
        armatureDef->AddPrimitive(animationResults.initialPoseReference);
        armatureDef->AddPrimitive(animationResults.boneParentReference);
        armatureDef->AddPrimitive(animationResults.boneCountMacro);
        armatureDef->AddPrimitive(animationResults.numberOfAttachmentMacros);

        fileDefinition.AddDataDefinition("armature", "struct SKArmatureDefinition", false, "_geo", std::move(armatureDef));
    }
    
    return result;
}