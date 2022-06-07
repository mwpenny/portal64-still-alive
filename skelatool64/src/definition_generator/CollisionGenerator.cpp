#include "CollisionGenerator.h"

#include "../StringUtils.h"
#include "../MathUtl.h"

#include <algorithm>

CollisionGrid::CollisionGrid(const aiAABB& boundaries) {
    x = floor(boundaries.mMin.x);
    z = floor(boundaries.mMin.z);

    spanX = ceil((boundaries.mMax.x - x) / COLLISION_GRID_CELL_SIZE);
    spanZ = ceil((boundaries.mMax.z - z) / COLLISION_GRID_CELL_SIZE);
    
    cells.resize(spanX);

    for (int i = 0; i < spanX; ++i) {
        cells[i].resize(spanZ);
    }
}

void CollisionGrid::AddToCells(const aiAABB& box, short value) {
    int minX = floor((box.mMin.x - x) / COLLISION_GRID_CELL_SIZE);
    int maxX = floor((box.mMax.x - x) / COLLISION_GRID_CELL_SIZE);
    int minZ = floor((box.mMin.z - z) / COLLISION_GRID_CELL_SIZE);
    int maxZ = floor((box.mMax.z - z) / COLLISION_GRID_CELL_SIZE);

    if (maxX < 0) maxX = 0;
    if (minX >= spanX) minX = spanX - 1;

    if (maxZ < 0) maxZ = 0;
    if (minX >= spanZ) minZ = spanZ - 1;

    for (int currX = minX; currX <= maxX; ++currX) {
        for (int currZ = minZ; currZ <= maxZ; ++currZ) {
            if (currX >= 0 && currX < spanX && currZ >= 0 && currZ < spanZ) {
                cells[currX][currZ].push_back(value);
            }
        }
    }
}

std::shared_ptr<CollisionGeneratorOutput> generateCollision(const aiScene* scene, CFileDefinition& fileDefinition, const DisplayListSettings& settings, RoomGeneratorOutput& roomOutput, NodeGroups& nodeGroups) {
    std::shared_ptr<CollisionGeneratorOutput> output(new CollisionGeneratorOutput());
    
    std::unique_ptr<StructureDataChunk> collidersChunk(new StructureDataChunk());
    std::unique_ptr<StructureDataChunk> colliderTypeChunk(new StructureDataChunk());
    std::unique_ptr<StructureDataChunk> collisionObjectChunk(new StructureDataChunk());

    aiMatrix4x4 globalTransform = settings.CreateCollisionTransform();

    int meshCount = 0;

    std::string quadCollidersName = fileDefinition.GetUniqueName("quad_colliders");
    std::string colliderTypesName = fileDefinition.GetUniqueName("collider_types");
    std::string collisionObjectsName = fileDefinition.GetUniqueName("collision_objects");

    std::vector<NodeWithArguments> nodes = nodeGroups.NodesForType("@collision");

    sortNodesWithArgsByRoom(nodes, roomOutput);

    std::vector<aiAABB> roomBoxes;
    std::vector<int> quadRooms;

    for (int i = 0; i < roomOutput.roomCount; ++i) {
        roomBoxes.push_back(aiAABB());
    }

    for (auto nodeInfo : nodes) {
        for (unsigned i = 0; i < nodeInfo.node->mNumMeshes; ++i) {
            aiMesh* mesh = scene->mMeshes[nodeInfo.node->mMeshes[i]];

            bool isTransparent = std::find(nodeInfo.arguments.begin(), nodeInfo.arguments.end(), "transparent") != nodeInfo.arguments.end();

            CollisionQuad collider(mesh, globalTransform * nodeInfo.node->mTransformation);
            collidersChunk->Add(std::move(collider.Generate()));

            std::unique_ptr<StructureDataChunk> colliderType(new StructureDataChunk());
            colliderType->AddPrimitive<const char*>("CollisionShapeTypeQuad");
            colliderType->AddPrimitive(std::string("&" + quadCollidersName + "[" + std::to_string(meshCount) + "]"));
            colliderType->AddPrimitive(0.0f);
            colliderType->AddPrimitive(1.0f);
            colliderType->AddPrimitive<const char*>("NULL");
            colliderTypeChunk->Add(std::move(colliderType));

            std::unique_ptr<StructureDataChunk> collisionObject(new StructureDataChunk());
            collisionObject->AddPrimitive(std::string("&" + colliderTypesName + "[" + std::to_string(meshCount) + "]"));
            collisionObject->AddPrimitive<const char*>("NULL");
            collisionObject->Add(std::unique_ptr<DataChunk>(new StructureDataChunk(collider.BoundingBox())));
            collisionObject->AddPrimitive<const char*>(isTransparent ? 
                "COLLISION_LAYERS_STATIC | COLLISION_LAYERS_TRANSPARENT | COLLISION_LAYERS_TANGIBLE" : 
                "COLLISION_LAYERS_STATIC | COLLISION_LAYERS_TANGIBLE");
            collisionObjectChunk->Add(std::move(collisionObject));


            output->quads.push_back(collider);

            int room = roomOutput.RoomForNode(nodeInfo.node);
            quadRooms.push_back(room);

            aiAABB bb = collider.BoundingBox();
            aiAABB& roomBox = roomBoxes[room];
            
            if (roomBox.mMin == roomBox.mMax) {
                roomBox = bb;
            } else {
                roomBox.mMin = min(roomBox.mMin, bb.mMin);
                roomBox.mMax = max(roomBox.mMax, bb.mMax);
            }

            ++meshCount;
        }
    }

    for (int i = 0; i < roomOutput.roomCount; ++i) {
        output->roomGrids.push_back(CollisionGrid(roomBoxes[i]));
    }

    int quadIndex = 0;
    for (auto& quad : output->quads) {
        output->roomGrids[quadRooms[quadIndex]].AddToCells(quad.BoundingBox(), quadIndex);
        quadIndex++;
    }

    std::unique_ptr<FileDefinition> collisionFileDef(new DataFileDefinition(
        "struct CollisionQuad", 
        quadCollidersName, 
        true, 
        "_geo", 
        std::move(collidersChunk)
    ));

    collisionFileDef->AddTypeHeader("\"physics/collision_quad.h\"");
    collisionFileDef->AddTypeHeader("\"physics/collision.h\"");
    collisionFileDef->AddTypeHeader("\"physics/collision_object.h\"");
    
    fileDefinition.AddDefinition(std::move(collisionFileDef));

    fileDefinition.AddDefinition(std::unique_ptr<FileDefinition>(new DataFileDefinition(
        "struct ColliderTypeData", 
        colliderTypesName, 
        true, 
        "_geo", 
        std::move(colliderTypeChunk)
    )));

    fileDefinition.AddDefinition(std::unique_ptr<FileDefinition>(new DataFileDefinition(
        "struct CollisionObject", 
        collisionObjectsName, 
        true, 
        "_geo", 
        std::move(collisionObjectChunk)
    )));

    fileDefinition.AddMacro(fileDefinition.GetMacroName("QUAD_COLLIDERS_COUNT"), std::to_string(meshCount));

    output->quadsName = collisionObjectsName;

    return output;
}