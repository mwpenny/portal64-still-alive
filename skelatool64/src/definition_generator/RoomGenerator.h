#ifndef __ROOM_GENERATOR_H__
#define __ROOM_GENERATOR_H__

#include "DefinitionGenerator.h"

#include <map>

struct NamedLocation {
    const aiNode* node;
    std::string name;
    int index;
};

struct RoomGeneratorOutput {
    std::map<const aiNode*, int> roomIndexMapping;
    std::vector<NamedLocation> namedLocations;
    int roomCount;

    short FindLocationRoom(const std::string& name) const;
};

void sortNodesByRoom(std::vector<aiNode*>& nodes, RoomGeneratorOutput& roomOutput);

class RoomGenerator : public DefinitionGenerator {
public:
    virtual bool ShouldIncludeNode(aiNode* node);
    virtual void GenerateDefinitions(const aiScene* scene, CFileDefinition& fileDefinition);

    const RoomGeneratorOutput& GetOutput() const;
private:
    RoomGeneratorOutput mOutput;
};

#endif