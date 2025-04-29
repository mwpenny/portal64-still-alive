#ifndef __COLLISION_CYLINDER_H__
#define __COLLISION_CYLINDER_H__

#include "collision_quad.h"
#include "collision.h"
#include "math/vector2.h"

struct CollisionCylinder {
    float radius;
    float halfHeight;
    struct Vector2* edgeVectors;
    int edgeCount;
    struct CollisionQuad* outsideFaces;
};

extern struct ColliderCallbacks gCollisionCylinderCallbacks;

#endif