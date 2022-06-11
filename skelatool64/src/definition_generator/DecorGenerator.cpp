#include "DecorGenerator.h"

void generateDecorDefinition(const aiScene* scene, CFileDefinition& fileDefinition, StructureDataChunk& levelDef, const RoomGeneratorOutput& roomOutput, const DisplayListSettings& settings, NodeGroups& nodeGroups) {
    std::unique_ptr<StructureDataChunk> decorDefs(new StructureDataChunk());

    aiMatrix4x4 baseTransfrom = settings.CreateCollisionTransform();

    int decorCount = 0;

    for (auto& nodeInfo : nodeGroups.NodesForType("@decor")) {
        if (nodeInfo.arguments.size() == 0) {
            continue;
        }

        std::unique_ptr<StructureDataChunk> singleDef(new StructureDataChunk());

        aiMatrix4x4 nodeTransform = baseTransfrom * nodeInfo.node->mTransformation;

        aiVector3D position;
        aiQuaternion rotation;
        aiVector3D scale;
        nodeTransform.Decompose(scale, rotation, position);
        singleDef->Add(std::unique_ptr<DataChunk>(new StructureDataChunk(position)));
        singleDef->Add(std::unique_ptr<DataChunk>(new StructureDataChunk(rotation)));
        singleDef->AddPrimitive(roomOutput.RoomForNode(nodeInfo.node));
        singleDef->AddPrimitive(std::string("DECOR_TYPE_") + nodeInfo.arguments[0]);

        decorDefs->Add(std::move(singleDef));
        ++decorCount;
    }

    std::string decorName = fileDefinition.GetUniqueName("decor");
    std::unique_ptr<FileDefinition> decorDef(new DataFileDefinition("struct DecorDefinition", decorName, true, "_geo", std::move(decorDefs)));
    decorDef->AddTypeHeader("\"decor/decor_object_list.h\"");
    fileDefinition.AddDefinition(std::move(decorDef));

    levelDef.AddPrimitive("decor", decorName);
    levelDef.AddPrimitive("decorCount", decorCount);
}