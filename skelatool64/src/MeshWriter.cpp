#include "MeshWriter.h"

#include <set>
#include <sstream>

#include "RCPState.h"
#include "DisplayListGenerator.h"
#include "StringUtils.h"

MaterialCollector::MaterialCollector(): mSceneCount(0) {}

void useTexture(std::set<std::shared_ptr<TextureDefinition>>& usedTextures, std::shared_ptr<TextureDefinition> texture, CFileDefinition& fileDefinition, const std::string& fileSuffix) {
    if (usedTextures.find(texture) != usedTextures.end()) {
        return;
    }

    usedTextures.insert(texture);

    fileDefinition.AddDefinition(std::move(texture->GenerateDefinition(fileDefinition.GetUniqueName(texture->Name()), fileSuffix)));
}

void MaterialCollector::UseMaterial(const std::string& material, DisplayListSettings& settings) {
    auto materialDL = settings.mMaterials.find(material);

    if (materialDL == settings.mMaterials.end()) {
        return;
    }

    auto prevCount = mMaterialUseCount.find(material);

    if (prevCount == mMaterialUseCount.end()) {
        mMaterialUseCount[material] = 1;
    } else {
        mMaterialUseCount[material] = prevCount->second + 1;
    }
}

void MaterialCollector::CollectMaterialResources(const aiScene* scene, std::vector<RenderChunk>& renderChunks, DisplayListSettings& settings) {
    for (auto chunk = renderChunks.begin(); chunk != renderChunks.end(); ++chunk) {
        UseMaterial(ExtendedMesh::GetMaterialName(scene->mMaterials[chunk->mMesh->mMesh->mMaterialIndex]), settings);
    }
    ++mSceneCount;
}

void MaterialCollector::GenerateMaterials(DisplayListSettings& settings, CFileDefinition& fileDefinition, const std::string& fileSuffix) {
    for (auto useCount = mMaterialUseCount.begin(); useCount != mMaterialUseCount.end(); ++useCount) {
        if (useCount->second > 1 || mSceneCount > 1) {
            auto material = settings.mMaterials.find(useCount->first);

            for (int i = 0; i < MAX_TILE_COUNT; ++i) {
                auto tile = &material->second->mState.tiles[i];
                if (tile->isOn && tile->texture && !settings.mDefaultMaterialState.IsTextureLoaded(tile->texture, tile->tmem)) {
                    useTexture(mUsedTextures, tile->texture, fileDefinition, fileSuffix);
                }
            }

            DisplayList materialDL(fileDefinition.GetUniqueName(useCount->first));
            material->second->Write(fileDefinition, settings.mDefaultMaterialState, materialDL.GetDataChunk());
            mMaterialNameMapping[useCount->first] = materialDL.GetName();
            
            auto dl = materialDL.Generate(fileSuffix);

            fileDefinition.AddDefinition(std::move(dl));
        }
    }
}

void generateMeshIntoDLWithMaterials(const aiScene* scene, CFileDefinition& fileDefinition, MaterialCollector* materials, std::vector<RenderChunk>& renderChunks, DisplayListSettings& settings, DisplayList &displayList, const std::string& modelSuffix) {
    RCPState rcpState(settings.mDefaultMaterialState, settings.mVertexCacheSize, settings.mMaxMatrixDepth, settings.mCanPopMultipleMatrices);
    std::set<std::shared_ptr<TextureDefinition>> usedTextures;

    for (auto chunk = renderChunks.begin(); chunk != renderChunks.end(); ++chunk) {
        if (materials) {
            std::string materialName = ExtendedMesh::GetMaterialName(scene->mMaterials[chunk->mMesh->mMesh->mMaterialIndex]);
            displayList.AddCommand(std::unique_ptr<DisplayListCommand>(new CommentCommand("Material " + materialName)));
            auto mappedMaterialName = materials->mMaterialNameMapping.find(materialName);

            if (mappedMaterialName != materials->mMaterialNameMapping.end()) {
                displayList.AddCommand(std::unique_ptr<DisplayListCommand>(new CallDisplayListByNameCommand(mappedMaterialName->second)));
            } else {
                auto material = settings.mMaterials.find(materialName);

                if (material != settings.mMaterials.end()) {
                    auto& materialState = rcpState.GetMaterialState();

                    for (int i = 0; i < MAX_TILE_COUNT; ++i) {
                        auto tile = &material->second->mState.tiles[i];
                        if (tile->isOn && tile->texture && !materialState.IsTextureLoaded(tile->texture, tile->tmem)) {
                            useTexture(usedTextures, tile->texture, fileDefinition, modelSuffix);
                        }
                    }

                    material->second->Write(fileDefinition, materialState, displayList.GetDataChunk());
                    applyMaterial(material->second->mState, materialState);
                }
            }
            
            displayList.AddCommand(std::unique_ptr<DisplayListCommand>(new CommentCommand("End Material " + materialName)));
        }

        std::string vertexBuffer = fileDefinition.GetVertexBuffer(
            chunk->mMesh, 
            Material::GetVertexType(chunk->mMaterial), 
            Material::TextureWidth(chunk->mMaterial),
            Material::TextureHeight(chunk->mMaterial),
            modelSuffix
        );
        generateGeometry(*chunk, rcpState, vertexBuffer, displayList, settings.mHasTri2);
    }
    rcpState.TraverseToBone(nullptr, displayList);

    generateMaterial(fileDefinition, rcpState.GetMaterialState(), settings.mDefaultMaterialState, displayList.GetDataChunk());
}


void generateMeshIntoDL(const aiScene* scene, CFileDefinition& fileDefinition, std::vector<RenderChunk>& renderChunks, DisplayListSettings& settings, DisplayList &displayList, const std::string& fileSuffix) {
    MaterialCollector materials;

    materials.GenerateMaterials(settings, fileDefinition, fileSuffix);

    generateMeshIntoDLWithMaterials(scene, fileDefinition, &materials, renderChunks, settings, displayList, fileSuffix);
}

std::string generateMesh(const aiScene* scene, CFileDefinition& fileDefinition, std::vector<RenderChunk>& renderChunks, DisplayListSettings& settings, const std::string& fileSuffix) {
    DisplayList displayList(fileDefinition.GetUniqueName("model_gfx"));
    generateMeshIntoDL(scene, fileDefinition, renderChunks, settings, displayList, fileSuffix);
    std::unique_ptr<FileDefinition> dlResult = displayList.Generate(fileSuffix);


    if (fileDefinition.GetBoneHierarchy().HasData()) {
        dlResult->AddTypeHeader("\"sk64/skelatool_defs.h\"");
    }

    fileDefinition.AddDefinition(std::move(dlResult));

    return displayList.GetName();
}