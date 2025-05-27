#ifndef __RAY_TRACING_H__
#define __RAY_TRACING_H__

#include <stdint.h>

#include "collision_object.h"
#include "collision_quad.h"
#include "math/ray.h"

struct RaycastHit {
    struct Vector3 at;
    struct Vector3 normal;
    float distance;
    struct CollisionObject* object;
    struct Transform* throughPortal;
    short roomIndex;
    uint64_t passedRooms;
    short numPortalsPassed;
};

int raycastQuadShape(struct CollisionQuad* quad, struct Ray* ray, float maxDistance, struct RaycastHit* contact);
int raycastQuad(struct CollisionObject* quadObject, struct Ray* ray, float maxDistance, struct RaycastHit* contact);
int raycastBox(struct CollisionObject* boxObject, struct Ray* ray, float maxDistance, struct RaycastHit* contact);
int raycastSphere(struct Vector3* position, float radius, struct Ray* ray, float maxDistance, float* rayDistance);

#endif