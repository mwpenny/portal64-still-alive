#include "raycasting.h"

#include "collision_box.h"
#include "collision_cylinder.h"
#include "line.h"
#include "math/mathf.h"

#define NEAR_EDGE_ZERO      0.001f
#define NEAR_DOT_ZERO       0.00001f
#define MIN_RAY_LENGTH      0.0001f

int raycastQuadShape(struct CollisionQuad* quad, struct Ray* ray, float maxDistance, struct RaycastHit* contact) {
    float normalDot = vector3Dot(&ray->dir, &quad->plane.normal);

    if (fabsf(normalDot) < NEAR_DOT_ZERO) {
        return 0;
    }

    contact->distance = -(vector3Dot(&ray->origin, &quad->plane.normal) + quad->plane.d) / normalDot;

    if (contact->distance < MIN_RAY_LENGTH || contact->distance > maxDistance) {
        return 0;
    }

    vector3AddScaled(&ray->origin, &ray->dir, contact->distance, &contact->at);

    if (collisionQuadDetermineEdges(&contact->at, quad)) {
        return 0;
    }

    if (normalDot > 0.0f) {
        vector3Negate(&quad->plane.normal, &contact->normal);
    } else {
        contact->normal = quad->plane.normal;
    }

    return 1;
}

int raycastQuad(struct CollisionObject* quadObject, struct Ray* ray, float maxDistance, struct RaycastHit* contact) {
    struct CollisionQuad* quad = (struct CollisionQuad*)quadObject->collider->data;

    int result = raycastQuadShape(quad, ray, maxDistance, contact);

    if (result) {
        contact->object = quadObject;
    }

    return result;
}

int raycastBox(struct CollisionObject* boxObject, struct Ray* ray, float maxDistance, struct RaycastHit* contact) {
    struct CollisionBox* box = (struct CollisionBox*)boxObject->collider->data;

    float distance = rayDetermineDistance(ray, boxObject->position);

    if (distance < 0.0f) {
        return 0;
    }

    struct Vector3 nearestPoint;

    vector3AddScaled(&ray->origin, &ray->dir, distance, &nearestPoint);

    if (vector3DistSqrd(boxObject->position, &nearestPoint) > vector3MagSqrd(&box->sideLength)) {
        return 0;
    }

    struct Ray localRay;
    collisionObjectLocalRay(boxObject, ray, &localRay);

    contact->distance = maxDistance;

    for (int i = 0; i < 3; ++i) {
        struct RaycastHit hitTest;

        // d = -(o * N + d) / (N * D)

        float dir = VECTOR3_AS_ARRAY(&localRay.dir)[i];

        if (fabsf(dir) < NEAR_DOT_ZERO) {
            continue;
        }

        if (dir > 0.0f) {
            hitTest.distance = -(VECTOR3_AS_ARRAY(&localRay.origin)[i] + VECTOR3_AS_ARRAY(&box->sideLength)[i]) / dir;
        } else {
            hitTest.distance = -(VECTOR3_AS_ARRAY(&localRay.origin)[i] - VECTOR3_AS_ARRAY(&box->sideLength)[i]) / dir;
        }

        // check if hit is within valid bounds
        if (hitTest.distance < MIN_RAY_LENGTH || hitTest.distance > contact->distance) {
            continue;
        }

        vector3AddScaled(&localRay.origin, &localRay.dir, hitTest.distance, &hitTest.at);

        // check if hit is on cube face
        if (fabsf(hitTest.at.x) > box->sideLength.x + NEAR_EDGE_ZERO ||
            fabsf(hitTest.at.y) > box->sideLength.y + NEAR_EDGE_ZERO ||
            fabsf(hitTest.at.z) > box->sideLength.z + NEAR_EDGE_ZERO) {
            continue;
        }

        contact->at = hitTest.at;
        contact->distance = hitTest.distance;
        contact->normal = gZeroVec;
        VECTOR3_AS_ARRAY(&contact->normal)[i] = dir < 0.0f ? 1.0f : -1.0f;
    }

    if (contact->distance != maxDistance) {
        contact->object = boxObject;
        transformPoint(&boxObject->body->transform, &contact->at, &contact->at);
        collisionObjectAddBodyOffset(boxObject, &contact->at);

        quatMultVector(&boxObject->body->transform.rotation, &contact->normal, &contact->normal);
    }

    contact->roomIndex = boxObject->body->currentRoom;

    return contact->distance != maxDistance;
}

int raycastSphere(struct Vector3* position, float radius, struct Ray* ray, float maxDistance, float* rayDistance) {
    struct Vector3 hyp;
    vector3Sub(position, &ray->origin, &hyp);

    // Distance along ray to sphere origin
    float distance = vector3Dot(&hyp, &ray->dir);
    if (distance < 0.0f || (distance - radius) > maxDistance) {
        return 0;
    }

    // Distance from sphere origin to projected point
    float normDist = vector3MagSqrd(&hyp) - (distance * distance);
    if (normDist > (radius * radius)) {
        return 0;
    }

    // Subtract distance from projected point to sphere edge
    //
    // Allow hitting behind the ray origin to handle spheres
    // next to portals being cast through
    distance -= sqrtf((radius * radius) - normDist);
    if (/*distance < 0.0f || */distance > maxDistance) {
        return 0;
    }

    *rayDistance = distance;
    return 1;
}
