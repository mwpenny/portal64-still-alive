#include "world.h"

int worldCheckDoorwayCrossings(struct World* world, struct Vector3* prevPosition, struct Vector3* position, int currentRoom) {
    if (currentRoom == RIGID_BODY_NO_ROOM) {
        return RIGID_BODY_NO_ROOM;
    }

    struct Room* room = &world->rooms[currentRoom];

    for (int i = 0; i < room->doorwayCount; ++i) {
        struct Doorway* doorway = &world->doorways[room->doorwayIndices[i]];

        int prevSide = planePointDistance(&doorway->quad.plane, prevPosition) > 0.0f;
        int currSide = planePointDistance(&doorway->quad.plane, position) > 0.0f;

        if (prevSide != currSide) {
            // Make sure vector from previous to current position is in bounds.
            // It's not enough to check the before/after position: fast objects
            // could pass through without being in bounds on one or both sides.
            struct Vector3 crossing;
            vector3Sub(position, prevPosition, &crossing);

            float distance;
            if (planeRayIntersection(&doorway->quad.plane, prevPosition, &crossing, &distance)) {
                vector3AddScaled(prevPosition, &crossing, distance, &crossing);
            } else {
                // Near-parallel crossing
                crossing = *position;
            }

            if (collisionQuadDetermineEdges(&crossing, &doorway->quad)) {
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