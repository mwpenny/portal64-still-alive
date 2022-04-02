#ifndef __COLLISION_QUAD_H__
#define __COLLISION_QUAD_H__

#include "../math/vector3.h"

struct CollisionQuad {
    struct Vector3 corner;
    struct Vector3 edgeA;
    float edgeALength;
    struct Vector3 edgeB;
    float edgeBLength;
    struct Vector3 normal;
};

#endif