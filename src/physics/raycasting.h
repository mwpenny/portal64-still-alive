#ifndef __RAY_TRACING_H__
#define __RAY_TRACING_H__

#include "collision_quad.h"
#include "collision_object.h"
#include "../math/ray.h"

struct RaycastHit {
    struct Vector3 at;
    struct Vector3 normal;
    float distance;
    struct CollisionObject* object;
    struct Transform* throughPortal;
    short roomIndex;
    short numPortalsPassed;
};

int raycastQuadShape(struct CollisionQuad* quad, struct Ray* ray, float maxDistance, struct RaycastHit* contact);
int raycastQuad(struct CollisionObject* quadObject, struct Ray* ray, float maxDistance, struct RaycastHit* contact);
int raycastBox(struct CollisionObject* boxObject, struct Ray* ray, float maxDistance, struct RaycastHit* contact);

#endif