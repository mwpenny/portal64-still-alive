#include "RoomGenerator.h"

#include <assimp/aabb.h>
#include "../StringUtils.h"
#include "../MathUtl.h"

#define ROOM_PREFIX "@room "

#define LOCATION_PREFIX "@location "

#define DOORWAY_PREFIX  "@doorway"

Doorway::Doorway(const aiNode* node, const CollisionQuad& quad): 
    node(node), quad(quad), roomA(0), roomB(0) {

}

RoomGenerator::RoomGenerator(const DisplayListSettings& settings): DefinitionGenerator(), mSettings(settings) {}

short RoomGeneratorOutput::FindLocationRoom(const std::string& name) const {
    for (auto& location : namedLocations) {
        if (location.name == name) {
            auto room = roomIndexMapping.find(location.node);

            if (room == roomIndexMapping.end()) {
                return -1;
            }

            return room->second;
        }
    }

    return -1;
}

struct RoomBlock {
    aiAABB boundingBox;
    int roomIndex;
};

bool RoomGenerator::ShouldIncludeNode(aiNode* node) {
    return true;
}

void sortNodesByRoom(std::vector<aiNode*>& nodes, RoomGeneratorOutput& roomOutput) {
    std::sort(nodes.begin(), nodes.end(), [&](const aiNode* a, const aiNode* b) -> bool {
        return roomOutput.roomIndexMapping[a] < roomOutput.roomIndexMapping[b];
    });
}

int findClosestRoom(const aiNode* node, const aiScene* scene, CFileDefinition& fileDefinition, const std::vector<RoomBlock>& roomBlocks, int ignoreRoom) {
    float distance = INFINITY;
    int closestRoom = 0;

    for (auto& roomBlock : roomBlocks) {
        if (roomBlock.roomIndex == ignoreRoom) {
            continue;
        }

        aiVector3D localCenter;

        if (node->mNumMeshes) {
            auto mesh = fileDefinition.GetExtendedMesh(scene->mMeshes[node->mMeshes[0]]);

            localCenter = (mesh->bbMax + mesh->bbMin) * 0.5f;
        }

        float roomDistance = distanceToAABB(roomBlock.boundingBox, node->mTransformation * localCenter);

        if (roomDistance < distance) {
            distance = roomDistance;
            closestRoom = roomBlock.roomIndex;
        }
    }

    return closestRoom;
}

void RoomGenerator::GenerateDefinitions(const aiScene* scene, CFileDefinition& fileDefinition) {
    std::vector<RoomBlock> roomBlocks;

    aiMatrix4x4 collisionTransform = mSettings.CreateCollisionTransform();

    mOutput.roomCount = 0;

    for (auto node : mIncludedNodes) {
        std::string nodeName(node->mName.C_Str());
        
        if (StartsWith(nodeName, ROOM_PREFIX) && node->mNumMeshes) {
            auto mesh = fileDefinition.GetExtendedMesh(scene->mMeshes[node->mMeshes[0]]);
            aiVector3D minPoint = node->mTransformation * mesh->bbMin;
            aiVector3D maxPoint = node->mTransformation * mesh->bbMax;

            struct RoomBlock roomBlock;
            roomBlock.boundingBox.mMin = min(minPoint, maxPoint);
            roomBlock.boundingBox.mMax = max(minPoint, maxPoint);
            roomBlock.roomIndex = std::atoi(nodeName.substr(strlen(ROOM_PREFIX)).c_str());
            roomBlocks.push_back(roomBlock);
        }

        if (StartsWith(nodeName, LOCATION_PREFIX)) {
            NamedLocation location;
            location.node = node;
            location.name = nodeName.substr(strlen(LOCATION_PREFIX));
            location.index = mOutput.namedLocations.size();

            mOutput.namedLocations.push_back(location);
        }

        if (StartsWith(nodeName, DOORWAY_PREFIX) && node->mNumMeshes) {
            mOutput.doorways.push_back(Doorway(node, CollisionQuad(
                scene->mMeshes[node->mMeshes[0]], 
                collisionTransform * node->mTransformation
            )));
        }
    }

    if (roomBlocks.size() == 0) {
        struct RoomBlock roomBlock;
        roomBlock.roomIndex = 0;
        roomBlocks.push_back(roomBlock);
    }

    for (auto node : mIncludedNodes) {
        int closestRoom = findClosestRoom(node, scene, fileDefinition, roomBlocks, -1);
        mOutput.roomIndexMapping[node] = closestRoom;
        mOutput.roomCount = std::max(mOutput.roomCount, closestRoom + 1);
    }

    for (auto& doorway : mOutput.doorways) {
        doorway.roomA = mOutput.roomIndexMapping[doorway.node];
        doorway.roomB = findClosestRoom(doorway.node, scene, fileDefinition, roomBlocks, doorway.roomA);

        mOutput.roomCount = std::max(mOutput.roomCount, doorway.roomB + 1);
    }
}

const RoomGeneratorOutput& RoomGenerator::GetOutput() const {
    return mOutput;
}