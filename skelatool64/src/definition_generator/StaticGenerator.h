#ifndef __STATIC_GENERATOR_H__
#define __STATIC_GENERATOR_H__

#include "DefinitionGenerator.h"
#include "../DisplayListSettings.h"
#include "./RoomGenerator.h"

struct StaticContentElement {
    std::string meshName;
    std::string materialName;
};

struct StaticGeneratorOutput {
    std::string staticContentName;
    std::vector<std::shared_ptr<ExtendedMesh>> staticMeshes;
    std::string roomMappingName;
};

class StaticGenerator : public DefinitionGenerator {
public:
    StaticGenerator(const DisplayListSettings& settings, const RoomGeneratorOutput& roomMapping);

    virtual bool ShouldIncludeNode(aiNode* node);
    virtual void GenerateDefinitions(const aiScene* scene, CFileDefinition& fileDefinition);

    const StaticGeneratorOutput& GetOutput() const;
private:
    DisplayListSettings mSettings;

    StaticGeneratorOutput mOutput;
    RoomGeneratorOutput mRoomMapping;
};

#endif