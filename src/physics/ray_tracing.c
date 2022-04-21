#include "ray_tracing.h"

#include "math/mathf.h"

#define NEAR_DOT_ZERO       0.00001f

int rayTraceQuad(struct Vector3* from, struct Vector3* dir, struct CollisionQuad* quad, struct RayTraceHit* contact) {
    float normalDot = vector3Dot(dir, &quad->plane.normal);

    if (fabsf(normalDot) < NEAR_DOT_ZERO) {
        return 0;
    }

    contact->distance = -(vector3Dot(from, &quad->plane.normal) + quad->plane.d) / normalDot;
    vector3AddScaled(from, dir, contact->distance, &contact->at);

    if (collisionQuadDetermineEdges(&contact->at, quad)) {
        return 0;
    }

    contact->normal = quad->plane.normal;

    return 1;
}