#include "RoomGenerator.h"

#include <assimp/aabb.h>
#include "../StringUtils.h"
#include "../MathUtl.h"

#define ROOM_PREFIX "@room"

#define LOCATION_PREFIX "@location"

#define DOORWAY_PREFIX  "@doorway"

#define DOOR_PREFIX "@door"

Doorway::Doorway(const aiNode* node, const CollisionQuad& quad): 
    node(node), quad(quad), roomA(0), roomB(0) {

}

Door::Door(const aiNode* node, int signalIndex): node(node), signalIndex(signalIndex) {}

short RoomGeneratorOutput::FindLocationRoom(const std::string& name) const {
    for (auto& location : namedLocations) {
        if (location.name == name) {
            return RoomForNode(location.node);
        }
    }

    return -1;
}

short RoomGeneratorOutput::FindLocationIndex(const std::string& name) const {
    for (auto& location : namedLocations) {
        if (location.name == name) {
            return location.index;
        }
    }

    return -1;
}

int RoomGeneratorOutput::RoomForNode(const aiNode* node) const {
    auto room = roomIndexMapping.find(node);

    if (room == roomIndexMapping.end()) {
        return -1;
    }

    return room->second;
}

struct RoomBlock {
    aiAABB boundingBox;
    int roomIndex;
};

void sortNodesByRoom(std::vector<aiNode*>& nodes, const RoomGeneratorOutput& roomOutput) {
    std::sort(nodes.begin(), nodes.end(), [&](const aiNode* a, const aiNode* b) -> bool {
        return roomOutput.RoomForNode(a) < roomOutput.RoomForNode(b);
    });
}

void sortNodesWithArgsByRoom(std::vector<NodeWithArguments>& nodes, const RoomGeneratorOutput& roomOutput) {
    std::sort(nodes.begin(), nodes.end(), [&](const NodeWithArguments& a, const NodeWithArguments& b) -> bool {
        return roomOutput.RoomForNode(a.node) < roomOutput.RoomForNode(b.node);
    });
}

int findClosestRoom(const aiNode* node, const aiScene* scene, CFileDefinition& fileDefinition, const std::vector<RoomBlock>& roomBlocks, int ignoreRoom, const RoomBlock** foundBlock = NULL) {
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
            if (foundBlock) {
                *foundBlock = &roomBlock;
            }
        }
    }

    return closestRoom;
}


std::shared_ptr<RoomGeneratorOutput> generateRooms(const aiScene* scene, CFileDefinition& fileDefinition, const DisplayListSettings& settings, Signals& signals, NodeGroups& nodeGroups) {
    std::vector<RoomBlock> roomBlocks;
    std::shared_ptr<RoomGeneratorOutput> output(new RoomGeneratorOutput());

    aiMatrix4x4 collisionTransform = settings.CreateCollisionTransform();

    output->roomCount = 0;

    for (auto& nodeInfo : nodeGroups.NodesForType(ROOM_PREFIX)) {
        if (!nodeInfo.node->mNumMeshes) {
            continue;
        }

        auto mesh = fileDefinition.GetExtendedMesh(scene->mMeshes[nodeInfo.node->mMeshes[0]]);
        aiVector3D minPoint = nodeInfo.node->mTransformation * mesh->bbMin;
        aiVector3D maxPoint = nodeInfo.node->mTransformation * mesh->bbMax;

        struct RoomBlock roomBlock;
        roomBlock.boundingBox.mMin = min(minPoint, maxPoint);
        roomBlock.boundingBox.mMax = max(minPoint, maxPoint);
        roomBlock.roomIndex = nodeInfo.arguments.size() ? std::atoi(nodeInfo.arguments[0].c_str()) : 0;
        roomBlocks.push_back(roomBlock);
    }

    for (auto& nodeInfo : nodeGroups.NodesForType(LOCATION_PREFIX)) {
        NamedLocation location;
        location.node = nodeInfo.node;
        if (nodeInfo.arguments.size()) {
            location.name = nodeInfo.arguments[0];
        } else {
            std::cout << "@location without a name";
        }
        location.index = output->namedLocations.size();

        output->namedLocations.push_back(location);
    }

    for (auto& nodeInfo : nodeGroups.NodesForType(DOORWAY_PREFIX)) {
        output->doorways.push_back(Doorway(nodeInfo.node, CollisionQuad(
            scene->mMeshes[nodeInfo.node->mMeshes[0]], 
            collisionTransform * nodeInfo.node->mTransformation
        )));
    }
    
    for (auto& nodeInfo : nodeGroups.NodesForType(DOOR_PREFIX)) {
        output->doors.push_back(Door(nodeInfo.node, nodeInfo.arguments.size() ? signals.SignalIndexForName(nodeInfo.arguments[0]): -1));
    }

    if (roomBlocks.size() == 0) {
        struct RoomBlock roomBlock;
        roomBlock.roomIndex = 0;
        roomBlocks.push_back(roomBlock);
    }

    forEachNode(scene->mRootNode, [&](aiNode* node) -> void {
        int closestRoom = findClosestRoom(node, scene, fileDefinition, roomBlocks, -1);
        output->roomIndexMapping[node] = closestRoom;
        output->roomCount = std::max(output->roomCount, closestRoom + 1);
    });

    for (auto& doorway : output->doorways) {
        doorway.roomA = output->roomIndexMapping[doorway.node];

        const RoomBlock* roomB = NULL;
        doorway.roomB = findClosestRoom(doorway.node, scene, fileDefinition, roomBlocks, doorway.roomA, &roomB);

        if (roomB) {
            aiVector3D roomBCenter = (roomB->boundingBox.mMin + roomB->boundingBox.mMax) * 0.5f;

            // check if the doorway is facing room A
            if ((roomBCenter - doorway.quad.corner) * doorway.quad.normal > 0.0f) {
                int tmp = doorway.roomA;
                doorway.roomA = doorway.roomB;
                doorway.roomB = tmp;
            }
        }

        output->roomCount = std::max(output->roomCount, doorway.roomB + 1);
    }

    for (auto& door : output->doors) {
        int doorwayIndex = -1;

        int index = 0;

        for (auto& doorway : output->doorways) {
            if (doorway.quad.IsCoplanar(collisionTransform * door.node->mTransformation * aiVector3D(0.0f, 0.0f, 0.0f))) {
                doorwayIndex = index;
                break;
            }

            ++index;
        }

        door.doorwayIndex = doorwayIndex;
    }

    return output;
}