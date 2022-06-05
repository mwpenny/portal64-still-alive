#ifndef __ROOM_GENERATOR_H__
#define __ROOM_GENERATOR_H__

#include "DefinitionGenerator.h"
#include "CollisionQuad.h"
#include "../DisplayListSettings.h"
#include "./Signals.h"

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

struct Door {
    Door(const aiNode* node, int signalIndex);
    
    const aiNode* node;
    int signalIndex;
    int doorwayIndex;
};

struct RoomGeneratorOutput {
    std::map<const aiNode*, int> roomIndexMapping;
    std::vector<NamedLocation> namedLocations;
    std::vector<Doorway> doorways;
    std::vector<Door> doors;
    int roomCount;

    short FindLocationRoom(const std::string& name) const;
    short FindLocationIndex(const std::string& name) const;
    int RoomForNode(const aiNode* node) const;
};

void sortNodesByRoom(std::vector<aiNode*>& nodes, const RoomGeneratorOutput& roomOutput);
void sortNodesWithArgsByRoom(std::vector<NodeWithArguments>& nodes, const RoomGeneratorOutput& roomOutput);

std::shared_ptr<RoomGeneratorOutput> generateRooms(const aiScene* scene, CFileDefinition& fileDefinition, const DisplayListSettings& settings, Signals& signals, NodeGroups& nodeGroups);

#endif