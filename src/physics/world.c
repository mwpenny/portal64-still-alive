#include "world.h"

int worldCheckDoorwaySides(struct World* world, struct Vector3* position, int currentRoom) {
    if (currentRoom == -1) {
        return 0;
    }

    struct Room* room = &world->rooms[currentRoom];

    int sideMask = 0;

    for (int i = 0; i < room->doorwayCount; ++i) {
        if (planePointDistance(&world->doorways[room->doorwayIndices[i]].quad.plane, position) > 0) {
            sideMask |= 1 << i;
        }
    }

    return sideMask;
}

int worldCheckDoorwayCrossings(struct World* world, struct Vector3* position, int currentRoom, int sideMask) {
    if (currentRoom == RIGID_BODY_NO_ROOM) {
        return RIGID_BODY_NO_ROOM;
    }

    struct Room* room = &world->rooms[currentRoom];

    for (int i = 0; i < room->doorwayCount; ++i) {
        struct Doorway* doorway = &world->doorways[room->doorwayIndices[i]];

        int prevSide = (sideMask & (1 << i)) != 0;
        int currSide = planePointDistance(&doorway->quad.plane, position) > 0;

        if (prevSide != currSide) {
            struct Vector3 posOnFace;
            planeProjectPoint(&doorway->quad.plane, position, &posOnFace);

            if (collisionQuadDetermineEdges(&posOnFace, &doorway->quad)) {
                continue;
            }

            return doorway->roomA == currentRoom ? doorway->roomB : doorway->roomA;
        }
    }

    return currentRoom;
}

float worldMaxDistanceInDirection(struct World* world, struct Ray* ray, u64 roomMask) {
    struct Vector3 maxInDir = ray->origin;

    for (int i = 0; i < world->roomCount; ++i) {
        if (!((1 << i) & roomMask)) {
            continue;
        }

        struct Vector3 current;
        box3DSupportFunction(&world->rooms[i].boundingBox, &ray->dir, &current);

        if (vector3Dot(&current, &ray->dir) > vector3Dot(&maxInDir, &ray->dir)) {
            maxInDir = current;
        }
    }

    return rayDetermineDistance(ray, &maxInDir);
}