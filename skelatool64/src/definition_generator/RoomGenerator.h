#ifndef __ROOM_GENERATOR_H__
#define __ROOM_GENERATOR_H__

#include "DefinitionGenerator.h"
#include "CollisionQuad.h"
#include "../DisplayListSettings.h"

#include <map>

struct NamedLocation {
    const aiNode* node;
    std::string name;
    int index;
};

struct Doorway {
    Doorway(const aiNode* node, const CollisionQuad& quad);
    const aiNode* node;
    CollisionQuad quad;
    int roomA;
    int roomB;
};

struct RoomGeneratorOutput {
    std::map<const aiNode*, int> roomIndexMapping;
    std::vector<NamedLocation> namedLocations;
    std::vector<Doorway> doorways;
    int roomCount;

    short FindLocationRoom(const std::string& name) const;
};

void sortNodesByRoom(std::vector<aiNode*>& nodes, RoomGeneratorOutput& roomOutput);

class RoomGenerator : public DefinitionGenerator {
public:
    RoomGenerator(const DisplayListSettings& settings);

    virtual bool ShouldIncludeNode(aiNode* node);
    virtual void GenerateDefinitions(const aiScene* scene, CFileDefinition& fileDefinition);

    const RoomGeneratorOutput& GetOutput() const;
private:
    DisplayListSettings mSettings;
    RoomGeneratorOutput mOutput;
};

#endif