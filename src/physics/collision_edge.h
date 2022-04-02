#ifndef __COLLISION_EDGE_H__
#define __COLLISION_EDGE_H__

#include "../math/vector3.h"

struct CollisionEdge {
    struct Vector3 endpoint;
    struct Vector3 direction;
    float length;
};

#endif