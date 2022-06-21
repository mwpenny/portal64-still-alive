#ifndef __COLLISION_GENERATOR_H__
#define __COLLISION_GENERATOR_H__

#include "DefinitionGenerator.h"
#include "../DisplayListSettings.h"
#include "CollisionQuad.h"
#include "RoomGenerator.h"

#define COLLISION_GRID_CELL_SIZE    4

struct CollisionGrid {
    CollisionGrid(const aiAABB& boundaries);

    short x;
    short z;
    short spanX;
    short spanZ;

    std::vector<std::vector<std::vector<short>>> cells;

    void AddToCells(const aiAABB& box, short value);
};

struct CollisionGeneratorOutput {
    std::string quadsName;
    std::vector<CollisionQuad> quads;
    std::vector<CollisionGrid> roomGrids;
};

std::shared_ptr<CollisionGeneratorOutput> generateCollision(const aiScene* scene, CFileDefinition& fileDefinition, const DisplayListSettings& settings, RoomGeneratorOutput* roomOutput, NodeGroups& nodeGroups);

void generateMeshCollider(CFileDefinition& fileDefinition, CollisionGeneratorOutput& collisionOutput);

#endif