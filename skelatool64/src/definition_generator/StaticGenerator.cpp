#include "StaticGenerator.h"

#include "../StringUtils.h"
#include "../MeshWriter.h"
#include "MeshDefinitionGenerator.h"
#include "MaterialGenerator.h"
#include "../RenderChunk.h"

std::shared_ptr<StaticGeneratorOutput> generateStatic(const aiScene* scene, CFileDefinition& fileDefinition, const DisplayListSettings& settings, RoomGeneratorOutput& roomMapping, NodeGroups& nodeGroups) {
    DisplayListSettings settingsCopy = settings;
    settingsCopy.mMaterials.clear();

    std::shared_ptr<StaticGeneratorOutput> output(new StaticGeneratorOutput());

    std::vector<StaticContentElement> elements;

    BoneHierarchy unusedBones;

    std::vector<NodeWithArguments> nodes = nodeGroups.NodesForType("@static");

    sortNodesWithArgsByRoom(nodes, roomMapping);

    for (auto& nodeInfo : nodes) {
        std::vector<RenderChunk> renderChunks;
        MeshDefinitionGenerator::AppendRenderChunks(scene, nodeInfo.node, fileDefinition, settings, renderChunks);

        for (auto& chunk : renderChunks) {
            StaticContentElement element;

            if (chunk.mMaterial) {
                settingsCopy.mDefaultMaterialState = chunk.mMaterial->mState;
                element.materialName = MaterialGenerator::MaterialIndexMacroName(chunk.mMaterial->mName);
            } else {
                element.materialName = "0";
            }
            std::vector<RenderChunk> singleChunk;
            singleChunk.push_back(chunk);
            element.meshName = generateMesh(scene, fileDefinition, singleChunk, settingsCopy, "_geo");

            elements.push_back(element);

            StaticMeshInfo meshInfo;
            meshInfo.staticMesh = chunk.mMesh;
            meshInfo.gfxName = element.meshName;
            meshInfo.material = chunk.mMaterial;

            output->staticMeshes.push_back(meshInfo);
            output->staticRooms.push_back(roomMapping.RoomForNode(nodeInfo.node));
        }
    }

    std::unique_ptr<StructureDataChunk> staticContentList(new StructureDataChunk());

    for (auto& it : elements) {
        std::unique_ptr<StructureDataChunk> element(new StructureDataChunk());

        element->AddPrimitive(it.meshName);
        element->AddPrimitive(it.materialName);

        staticContentList->Add(std::move(element));
    }

    output->staticContentName = fileDefinition.GetUniqueName("static");

    std::unique_ptr<FileDefinition> fileDef(new DataFileDefinition("struct StaticContentElement", output->staticContentName, true, "_geo", std::move(staticContentList)));
    fileDef->AddTypeHeader("\"levels/level_def_gen.h\"");

    fileDefinition.AddDefinition(std::move(fileDef));

    std::unique_ptr<StructureDataChunk> roomToStaticMapping(new StructureDataChunk());
    int prevIndex = 0;
    for (int roomIndex = 0; roomIndex < roomMapping.roomCount; ++roomIndex) {
        std::unique_ptr<StructureDataChunk> singleMapping(new StructureDataChunk());

        int currentIndex = prevIndex;
        while (currentIndex < (int)output->staticRooms.size() && output->staticRooms[currentIndex] <= roomIndex) {
            ++currentIndex;
        }

        singleMapping->AddPrimitive(prevIndex);
        singleMapping->AddPrimitive(currentIndex);

        prevIndex = currentIndex;

        roomToStaticMapping->Add(std::move(singleMapping));
    }

    output->roomMappingName = fileDefinition.GetUniqueName("static_room_mapping");
    fileDefinition.AddDefinition(std::unique_ptr<FileDefinition>(new DataFileDefinition("struct Rangeu16", output->roomMappingName, true, "_geo", std::move(roomToStaticMapping))));

    return output;
}