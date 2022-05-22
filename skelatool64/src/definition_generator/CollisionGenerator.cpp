#include "CollisionGenerator.h"

#include "../StringUtils.h"

#include <algorithm>

CollisionGenerator::CollisionGenerator(const DisplayListSettings& settings, const RoomGeneratorOutput& roomOutput) : 
    DefinitionGenerator(), 
    mSettings(settings),
    mRoomOutput(roomOutput) {}


bool CollisionGenerator::ShouldIncludeNode(aiNode* node) {
    return StartsWith(node->mName.C_Str(), "@collision");
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

    for (int i = 0; i < mRoomOutput.roomCount; ++i) {
        mOutput.broadphaseIndex.push_back(std::vector<BroadphaseEdge>());
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

            aiAABB bb = collider.BoundingBox();
            BroadphaseEdge edge;
            edge.quadIndex = meshCount;

            edge.value = bb.mMin.x;
            mOutput.broadphaseIndex[mRoomOutput.RoomForNode(node)].push_back(edge);

            if (bb.mMin.x != bb.mMax.x) {
                edge.value = bb.mMax.x;
                mOutput.broadphaseIndex[mRoomOutput.RoomForNode(node)].push_back(edge);
            }

            ++meshCount;
        }
    }

    for (auto& broadphase : mOutput.broadphaseIndex) {
        std::sort(broadphase.begin(), broadphase.end(), [](const BroadphaseEdge& a, const BroadphaseEdge& b) -> bool {
            return a.value < b.value;
        });
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