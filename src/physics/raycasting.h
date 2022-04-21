#ifndef __RAY_TRACING_H__
#define __RAY_TRACING_H__

#include "collision_quad.h"
#include "collision_object.h"

struct RaycastHit {
    struct Vector3 at;
    struct Vector3 normal;
    float distance;
    struct CollisionObject* object;
    struct Transform* throughPortal;
};

int raycastQuad(struct Vector3* from, struct Vector3* dir, float maxDistance, struct CollisionQuad* quad, struct RaycastHit* contact);

#endif