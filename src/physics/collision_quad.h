#ifndef __COLLISION_QUAD_H__
#define __COLLISION_QUAD_H__

#include "../math/vector3.h"
#include "../math/plane.h"
#include "../math/transform.h"
#include "contact_solver.h"

struct CollisionQuad {
    struct Vector3 corner;
    struct Vector3 edgeA;
    float edgeALength;
    struct Vector3 edgeB;
    float edgeBLength;
    struct Plane plane;
    // used to filter out concave edges or
    // reduce duplicate checks for faces
    // that share edges
    u8 enabledEdges;
};

int collisionQuadDetermineEdges(struct Vector3* worldPoint, struct CollisionQuad* quad);
int collisionBoxCollideQuad(void* data, struct Transform* boxTransform, struct CollisionQuad* quad, struct ContactConstraintState* output);

#endif