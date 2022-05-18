#include "MaterialGenerator.h"

#include "../StringUtils.h"

MaterialGenerator::MaterialGenerator(const DisplayListSettings& settings): mSettings(settings) {}


bool MaterialGenerator::ShouldIncludeNode(aiNode* node) {
    return false;
}

void MaterialGenerator::GenerateDefinitions(const aiScene* scene, CFileDefinition& fileDefinition) {
    std::set<std::shared_ptr<TextureDefinition>> textures;

    for (auto& entry : mSettings.mMaterials) {
        for (int i = 0; i < 8; ++i) {
            if (entry.second->mState.tiles[i].texture) {
                textures.insert(entry.second->mState.tiles[i].texture);
            }
        }
    }

    for (auto& texture : textures) {
        fileDefinition.AddDefinition(std::move(texture->GenerateDefinition(fileDefinition.GetUniqueName(texture->Name()), "_mat")));
    }
    
    int index = 0;

    std::unique_ptr<StructureDataChunk> materialList(new StructureDataChunk());
    std::unique_ptr<StructureDataChunk> revertList(new StructureDataChunk());

    for (auto& entry : mSettings.mMaterials) {
        std::string name = fileDefinition.GetUniqueName(entry.first);

        DisplayList dl(name);
        entry.second->Write(fileDefinition, mSettings.mDefaultMaterialState, dl.GetDataChunk());
        std::unique_ptr<FileDefinition> material = dl.Generate("_mat");
        materialList->AddPrimitive(material->GetName());
        fileDefinition.AddDefinition(std::move(material));

        std::string revertName = fileDefinition.GetUniqueName(entry.first + "_revert");
        DisplayList revertDL(revertName);
        generateMaterial(fileDefinition, entry.second->mState, mSettings.mDefaultMaterialState, revertDL.GetDataChunk());
        std::unique_ptr<FileDefinition> materialRevert = revertDL.Generate("_mat");
        revertList->AddPrimitive(materialRevert->GetName());
        fileDefinition.AddDefinition(std::move(materialRevert));

        fileDefinition.AddMacro(MaterialIndexMacroName(entry.second->mName), std::to_string(index));

        ++index;
    }

    fileDefinition.AddMacro(fileDefinition.GetMacroName("MATERIAL_COUNT"), std::to_string(index));

    fileDefinition.AddDefinition(std::unique_ptr<FileDefinition>(new DataFileDefinition("Gfx*", fileDefinition.GetUniqueName("material_list"), true, "_mat", std::move(materialList))));
    fileDefinition.AddDefinition(std::unique_ptr<FileDefinition>(new DataFileDefinition("Gfx*", fileDefinition.GetUniqueName("material_revert_list"), true, "_mat", std::move(revertList))));
}

std::string MaterialGenerator::MaterialIndexMacroName(const std::string& materialName) {
    std::string result = materialName;
    std::transform(materialName.begin(), materialName.end(), result.begin(), ::toupper);
    makeCCompatible(result);
    return result + "_INDEX";
}