#ifndef __WORLD_H___
#define __WORLD_H___

#include "collision_object.h"
#include "../math/range.h"

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

    short* doorwayIndices;
    short doorwayCount;
};

struct World {
    struct Room* rooms;
    struct Doorway* doorways;
    short roomCount;
    short doorwayCount;
};

int worldCheckDoorwaySides(struct World* world, struct Vector3* position, int currentRoom);
int worldCheckDoorwayCrossings(struct World* world, struct Vector3* position, int currentRoom, int sideMask);

#endif