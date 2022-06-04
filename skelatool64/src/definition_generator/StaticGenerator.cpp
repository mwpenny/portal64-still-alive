#include "StaticGenerator.h"

#include "../StringUtils.h"
#include "../MeshWriter.h"
#include "MeshDefinitionGenerator.h"
#include "MaterialGenerator.h"
#include "../RenderChunk.h"

StaticGenerator::StaticGenerator(const DisplayListSettings& settings, const RoomGeneratorOutput& roomMapping) : DefinitionGenerator(), mSettings(settings), mRoomMapping(roomMapping) {

}

bool StaticGenerator::ShouldIncludeNode(aiNode* node) {
    return StartsWith(node->mName.C_Str(), "@static ") && node->mNumMeshes > 0;
}

void StaticGenerator::GenerateDefinitions(const aiScene* scene, CFileDefinition& fileDefinition) {
    DisplayListSettings settings = mSettings;
    settings.mMaterials.clear();

    std::vector<StaticContentElement> elements;

    BoneHierarchy unusedBones;

    sortNodesByRoom(mIncludedNodes, mRoomMapping);

    for (auto node = mIncludedNodes.begin(); node != mIncludedNodes.end(); ++node) {
        std::vector<RenderChunk> renderChunks;
        MeshDefinitionGenerator::AppendRenderChunks(scene, *node, fileDefinition, mSettings, renderChunks);

        for (auto& chunk : renderChunks) {
            StaticContentElement element;

            if (chunk.mMaterial) {
                settings.mDefaultMaterialState = chunk.mMaterial->mState;
                element.materialName = MaterialGenerator::MaterialIndexMacroName(chunk.mMaterial->mName);
            } else {
                element.materialName = "0";
            }
            std::vector<RenderChunk> singleChunk;
            singleChunk.push_back(chunk);
            element.meshName = generateMesh(scene, fileDefinition, singleChunk, settings, "_geo");

            elements.push_back(element);

            mOutput.staticMeshes.push_back(chunk.mMesh);
            mOutput.staticRooms.push_back(mRoomMapping.RoomForNode(*node));
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

    std::unique_ptr<StructureDataChunk> roomToStaticMapping(new StructureDataChunk());
    int prevIndex = 0;
    for (int roomIndex = 0; roomIndex < mRoomMapping.roomCount; ++roomIndex) {
        std::unique_ptr<StructureDataChunk> singleMapping(new StructureDataChunk());

        int currentIndex = prevIndex;
        while (currentIndex < (int)mOutput.staticRooms.size() && mOutput.staticRooms[currentIndex] <= roomIndex) {
            ++currentIndex;
        }

        singleMapping->AddPrimitive(prevIndex);
        singleMapping->AddPrimitive(currentIndex);

        prevIndex = currentIndex;

        roomToStaticMapping->Add(std::move(singleMapping));
    }

    mOutput.roomMappingName = fileDefinition.GetUniqueName("static_room_mapping");
    fileDefinition.AddDefinition(std::unique_ptr<FileDefinition>(new DataFileDefinition("struct Rangeu16", mOutput.roomMappingName, true, "_geo", std::move(roomToStaticMapping))));
}

const StaticGeneratorOutput& StaticGenerator::GetOutput() const {
    return mOutput;
}