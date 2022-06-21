#ifndef __COLLISION_QUAD_H__
#define __COLLISION_QUAD_H__

#include "../math/vector3.h"
#include "../math/plane.h"
#include "../math/transform.h"
#include "contact_solver.h"

#define POINT_NO_OVERLAP    -1

struct CollisionEdge {
    struct Vector3 origin;
    struct Vector3 direction;
    float length;
};

struct CollisionQuad {
    struct Vector3 corner;
    struct Vector3 edgeA;
    float edgeALength;
    struct Vector3 edgeB;
    float edgeBLength;
    struct Plane plane;
    float thickness;
};

void collisionQuadInitializeNormalContact(struct CollisionQuad* quad, struct ContactManifold* output);
int collisionQuadDetermineEdges(struct Vector3* worldPoint, struct CollisionQuad* quad);

#endif