#ifndef __WORLD_H___
#define __WORLD_H___

#include <stdint.h>

#include "collision_object.h"
#include "math/box3d.h"
#include "math/range.h"
#include "math/ray.h"

#define ROOM_FLAG_FROM_INDEX(index) (1LL << (index))
#define ROOM_NONE                   0xffff

enum DoorwayFlags {
    DoorwayFlagsOpen = (1 << 0),
};

struct Doorway {
    struct CollisionQuad quad;
    short roomA;
    short roomB;
    short flags;
};

struct Room {
    short* quadIndices;
    struct Rangeu16* cellContents;
    short spanX;
    short spanZ;
    short cornerX;
    short cornerZ;

    struct Box3D boundingBox;

    short* doorwayIndices;
    short doorwayCount;

    uint64_t nonVisibleRooms;
};

struct World {
    struct Room* rooms;
    struct Doorway* doorways;
    short roomCount;
    short doorwayCount;
};

int worldCheckDoorwayCrossings(struct World* world, struct Vector3* prevPosition, struct Vector3* position, int currentRoom);

float worldMaxDistanceInDirection(struct World* world, struct Ray* ray, uint64_t roomMask);

#endif
