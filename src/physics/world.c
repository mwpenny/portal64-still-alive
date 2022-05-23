#include "world.h"

int worldCheckDoorwaySides(struct World* world, struct Vector3* position, int currentRoom) {
    struct Room* room = &world->rooms[currentRoom];

    int sideMask = 0;

    for (int i = 0; i < room->doorwayCount; ++i) {
        if (planePointDistance(&world->doorways[room->doorwayIndices[i]].quad.plane, position) > 0) {
            sideMask |= 1 << 1;
        }
    }

    return sideMask;
}

int worldCheckDoorwayCrossings(struct World* world, struct Vector3* position, int currentRoom, int sideMask) {
    struct Room* room = &world->rooms[currentRoom];

    for (int i = 0; i < room->doorwayCount; ++i) {
        struct Doorway* doorway = &world->doorways[room->doorwayIndices[i]];

        int prevSide = (sideMask & (1 << 1)) != 0;
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