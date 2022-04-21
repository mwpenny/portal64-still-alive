#include "raycasting.h"

#include "math/mathf.h"

#define NEAR_DOT_ZERO       0.00001f
#define MIN_RAY_LENGTH      0.05f

int raycastQuad(struct Vector3* from, struct Vector3* dir, float maxDistance, struct CollisionQuad* quad, struct RaycastHit* contact) {
    float normalDot = vector3Dot(dir, &quad->plane.normal);

    if (fabsf(normalDot) < NEAR_DOT_ZERO) {
        return 0;
    }

    contact->distance = -(vector3Dot(from, &quad->plane.normal) + quad->plane.d) / normalDot;

    if (contact->distance < MIN_RAY_LENGTH || contact->distance > maxDistance) {
        return 0;
    }

    vector3AddScaled(from, dir, contact->distance, &contact->at);

    if (collisionQuadDetermineEdges(&contact->at, quad)) {
        return 0;
    }

    contact->normal = quad->plane.normal;

    return 1;
}