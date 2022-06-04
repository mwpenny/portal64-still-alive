#include "CollisionGenerator.h"

#include "../StringUtils.h"
#include "../MathUtl.h"

#include <algorithm>

CollisionGenerator::CollisionGenerator(const DisplayListSettings& settings, const RoomGeneratorOutput& roomOutput) : 
    DefinitionGenerator(), 
    mSettings(settings),
    mRoomOutput(roomOutput) {}


bool CollisionGenerator::ShouldIncludeNode(aiNode* node) {
    return StartsWith(node->mName.C_Str(), "@collision ");
}

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

void CollisionGenerator::GenerateDefinitions(const aiScene* scene, CFileDefinition& fileDefinition) {
    std::unique_ptr<StructureDataChunk> collidersChunk(new StructureDataChunk());
    std::unique_ptr<StructureDataChunk> colliderTypeChunk(new StructureDataChunk());
    std::unique_ptr<StructureDataChunk> collisionObjectChunk(new StructureDataChunk());

    aiMatrix4x4 globalTransform = mSettings.CreateCollisionTransform();

    int meshCount = 0;

    std::string quadCollidersName = fileDefinition.GetUniqueName("quad_colliders");
    std::string colliderTypesName = fileDefinition.GetUniqueName("collider_types");
    std::string collisionObjectsName = fileDefinition.GetUniqueName("collision_objects");

    sortNodesByRoom(mIncludedNodes, mRoomOutput);

    std::vector<aiAABB> roomBoxes;
    std::vector<int> quadRooms;

    for (int i = 0; i < mRoomOutput.roomCount; ++i) {
        roomBoxes.push_back(aiAABB());
    }

    for (auto node : mIncludedNodes) {
        for (unsigned i = 0; i < node->mNumMeshes; ++i) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

            CollisionQuad collider(mesh, globalTransform * node->mTransformation);
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
            collisionObjectChunk->Add(std::move(collisionObject));


            mOutput.quads.push_back(collider);

            int room = mRoomOutput.RoomForNode(node);
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

    for (int i = 0; i < mRoomOutput.roomCount; ++i) {
        mOutput.roomGrids.push_back(CollisionGrid(roomBoxes[i]));
    }

    int quadIndex = 0;
    for (auto& quad : mOutput.quads) {
        mOutput.roomGrids[quadRooms[quadIndex]].AddToCells(quad.BoundingBox(), quadIndex);
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

    mOutput.quadsName = collisionObjectsName;
}

const CollisionGeneratorOutput& CollisionGenerator::GetOutput() const {
    return mOutput;
}