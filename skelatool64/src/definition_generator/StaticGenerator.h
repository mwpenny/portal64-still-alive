#ifndef __STATIC_GENERATOR_H__
#define __STATIC_GENERATOR_H__

#include "DefinitionGenerator.h"
#include "../DisplayListSettings.h"
#include "./RoomGenerator.h"
#include "../RenderChunk.h"

struct StaticContentElement {
    std::string meshName;
    std::string materialName;
};

struct StaticMeshInfo {
    std::shared_ptr<ExtendedMesh> staticMesh;
    std::string gfxName;
    Material* material;
};

struct StaticGeneratorOutput {
    std::string staticContentName;
    std::vector<StaticMeshInfo> staticMeshes;
    std::vector<int> staticRooms;
    std::string roomMappingName;
};

std::shared_ptr<StaticGeneratorOutput> generateStatic(const aiScene* scene, CFileDefinition& fileDefinition, const DisplayListSettings& settings, RoomGeneratorOutput& roomMapping, NodeGroups& nodeGroups);

#endif