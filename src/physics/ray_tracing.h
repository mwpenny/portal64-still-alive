#ifndef __RAY_TRACING_H__
#define __RAY_TRACING_H__

#include "collision_quad.h"
#include "collision_object.h"

struct RayTraceHit {
    struct Vector3 at;
    struct Vector3 normal;
    float distance;
    struct CollisionObject* object;
};

int rayTraceQuad(struct Vector3* from, struct Vector3* dir, struct CollisionQuad* quad, struct RayTraceHit* contact);

#endif