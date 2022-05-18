#include "StaticGenerator.h"

#include "../StringUtils.h"
#include "../MeshWriter.h"
#include "MeshDefinitionGenerator.h"
#include "MaterialGenerator.h"
#include "../RenderChunk.h"

StaticGenerator::StaticGenerator(const DisplayListSettings& settings) : DefinitionGenerator(), mSettings(settings) {

}

bool StaticGenerator::ShouldIncludeNode(aiNode* node) {
    return StartsWith(node->mName.C_Str(), "@static") && node->mNumMeshes > 0;
}

void StaticGenerator::GenerateDefinitions(const aiScene* scene, CFileDefinition& fileDefinition) {
    DisplayListSettings settings = mSettings;
    settings.mMaterials.clear();

    std::vector<StaticContentElement> elements;

    for (auto node = mIncludedNodes.begin(); node != mIncludedNodes.end(); ++node) {
        std::vector<RenderChunk> renderChunks;
        MeshDefinitionGenerator::AppendRenderChunks(scene, *node, fileDefinition, mSettings, renderChunks);
        
        if (renderChunks.size()) {
            StaticContentElement element;

            if (renderChunks[0].mMaterial) {
                settings.mDefaultMaterialState = renderChunks[0].mMaterial->mState;
                element.materialName = MaterialGenerator::MaterialIndexMacroName(renderChunks[0].mMaterial->mName);
            } else {
                element.materialName = "0";
            }
            element.meshName = generateMesh(scene, fileDefinition, renderChunks, settings, "_geo");

            elements.push_back(element);

            mOutput.staticMeshes.push_back(renderChunks[0].mMesh);
        }
    }

    std::unique_ptr<StructureDataChunk> staticContentList(new StructureDataChunk());

    for (auto& it : elements) {
        std::unique_ptr<StructureDataChunk> element(new StructureDataChunk());

        element->AddPrimitive(it.meshName);
        element->AddPrimitive(it.materialName);

        staticContentList->Add(std::move(element));
    }

    mOutput.staticContentName = fileDefinition.GetUniqueName("static");

    std::unique_ptr<FileDefinition> fileDef(new DataFileDefinition("struct StaticContentElement", mOutput.staticContentName, true, "_geo", std::move(staticContentList)));
    fileDef->AddTypeHeader("\"levels/level_def_gen.h\"");
    fileDefinition.AddDefinition(std::move(fileDef));

}

const StaticGeneratorOutput& StaticGenerator::GetOutput() const {
    return mOutput;
}