#include "collision_capsule.h"

#include "collision_object.h"
#include "line.h"
#include "math/mathf.h"
#include "raycasting.h"

float collisionCapsuleSolidMofI(struct ColliderTypeData* typeData, float mass) {
    struct CollisionCapsule* capsule = (struct CollisionCapsule*)typeData->data;
    return (2.0f / 5.0f) * mass * capsule->radius * capsule->radius;
}

void collisionCapsuleBoundingBox(struct ColliderTypeData* typeData, struct Transform* transform, struct Box3D* box) {
    struct CollisionCapsule* capsule = (struct CollisionCapsule*)typeData->data;
    box->min.x = transform->position.x - capsule->radius;
    box->min.y = transform->position.y - capsule->radius - capsule->extendDownward;
    box->min.z = transform->position.z - capsule->radius;

    box->max.x = transform->position.x + capsule->radius;
    box->max.y = transform->position.y + capsule->radius;
    box->max.z = transform->position.z + capsule->radius;
}

#define SQRT_3  0.577350269f
#define SQRT_2  0.707106781f

static struct Vector2 gUnitCircle[] = {
    {1.0f, 0.0f},
    {SQRT_2, SQRT_2},
    {0.0f, 1.0f},
    {-SQRT_2, SQRT_2},
    {-1.0f, 0.0f},
    {-SQRT_2, -SQRT_2},
    {0.0f, -1.0f},
    {SQRT_2, -SQRT_2},
};

#define OFFSET_IN_CIRCLE(current, amount)  (((current) + (amount)) & 0x7)

int collisionCapsuleMinkowsiSum(void* data, struct Basis* basis, struct Vector3* direction, struct Vector3* output) {
    struct CollisionCapsule* capsule = (struct CollisionCapsule*)data;

    float directionY = vector3Dot(&basis->y, direction);

    if (directionY * directionY > 0.5f * vector3MagSqrd(direction)) {
        if (directionY > 0.0f) {
            vector3Scale(&basis->y, output, capsule->radius);
            return 0xFF;
        } else {
            vector3Scale(&basis->y, output, -capsule->radius - capsule->extendDownward);
            return 0xFF00;
        }
    } else {
        struct Vector2 horizontalBasis;

        horizontalBasis.x = vector3Dot(&basis->x, direction);
        horizontalBasis.y = vector3Dot(&basis->z, direction);

        int circleIndex = 0;
        float currentDot = vector2Dot(&gUnitCircle[0], &horizontalBasis);

        if (currentDot < 0.0f) {
            circleIndex = 4;
            currentDot = -currentDot;
        }

        for (int i = 2; i >=1; --i) {
            int nextIndex = OFFSET_IN_CIRCLE(circleIndex, i);
            int prevIndex = OFFSET_IN_CIRCLE(circleIndex, -i);

            float nextDot = vector2Dot(&gUnitCircle[nextIndex], &horizontalBasis);
            float prevDot = vector2Dot(&gUnitCircle[prevIndex], &horizontalBasis);

            if (nextDot > currentDot) {
                circleIndex = nextIndex;
                currentDot = nextDot;
            }

            if (prevDot > currentDot) {
                circleIndex = prevIndex;
                currentDot = prevDot;
            }
        }

        int result;

        if (circleIndex == 0) {
            result = 0x81;
        } else {
            result = 0xC0 >> (7 - circleIndex);
        }

        vector3Scale(&basis->x, output, gUnitCircle[circleIndex].x * capsule->radius);
        vector3AddScaled(output, &basis->z, gUnitCircle[circleIndex].y * capsule->radius, output);

        if (directionY < 0.0f) {
            vector3AddScaled(output, &basis->y, -capsule->extendDownward, output);
            result <<= 8;
        }

        return result;
    }
}

static void finishContact(struct CollisionObject* capsuleObject, struct Ray* ray, struct RaycastHit* contact) {
    vector3AddScaled(&ray->origin, &ray->dir, contact->distance, &contact->at);
    vector3Negate(&ray->dir, &contact->normal);  // Approximate to save a sqrtf()
    contact->object = capsuleObject;
    contact->roomIndex = capsuleObject->body->currentRoom;
}

static int collisionCapsuleRaycastCap(struct CollisionObject* capsuleObject, struct Ray* ray, uint8_t checkTop, float maxDistance, struct RaycastHit* contact) {
    struct CollisionCapsule* capsule = (struct CollisionCapsule*)capsuleObject->collider->data;
    struct Vector3* position = &capsuleObject->body->transform.position;

    if (checkTop) {
        raycastSphere(position, capsule->radius, ray, maxDistance, &contact->distance);
    } else {
        struct Vector3 bottomSpherePos;
        vector3AddScaled(
            position,
            &capsuleObject->body->rotationBasis.y,
            -capsule->extendDownward,
            &bottomSpherePos
        );
        raycastSphere(&bottomSpherePos, capsule->radius, ray, maxDistance, &contact->distance);
    }

    if (contact->distance == maxDistance) {
        return 0;
    }

    finishContact(capsuleObject, ray, contact);
    return 1;
}

int collisionCapsuleRaycast(struct CollisionObject* capsuleObject, struct Ray* ray, float maxDistance, struct RaycastHit* contact) {
    struct CollisionCapsule* capsule = (struct CollisionCapsule*)capsuleObject->collider->data;
    struct Vector3* capsulePos = &capsuleObject->body->transform.position;

    float distance = rayDetermineDistance(ray, capsulePos);
    if (distance < 0.0f) {
        return 0;
    }

    float coarseRadius = capsule->radius + capsule->extendDownward;
    if ((distance - coarseRadius) > maxDistance) {
        return 0;
    }

    struct Vector3 nearestPoint;
    vector3AddScaled(&ray->origin, &ray->dir, distance, &nearestPoint);
    if (vector3DistSqrd(capsulePos, &nearestPoint) > (coarseRadius * coarseRadius)) {
        return 0;
    }

    struct Vector3* capsuleUp = &capsuleObject->body->rotationBasis.y;

    float rayCapsuleDot = vector3Dot(&ray->dir, capsuleUp);
    if (fabsf(rayCapsuleDot) > 0.999f) {
        // Ray passes through center of capsule
        return collisionCapsuleRaycastCap(
            capsuleObject,
            ray,
            rayCapsuleDot < 0.0f,  // checkTop
            maxDistance,
            contact
        );
    }

    // Check infinite cylinder using ray on same local XZ plane
    struct Vector3 rayToCapsule;
    struct Ray ray2d;
    vector3Sub(capsulePos, &ray->origin, &rayToCapsule);
    vector3AddScaled(&ray->origin, capsuleUp, vector3Dot(capsuleUp, &rayToCapsule), &ray2d.origin);
    vector3ProjectPlane(&ray->dir, capsuleUp, &ray2d.dir);
    vector3Normalize(&ray2d.dir, &ray2d.dir);
    if (!raycastSphere(capsulePos, capsule->radius, &ray2d, maxDistance, &contact->distance)) {
        return 0;
    }

    // Potential hit: check 3D intersection point
    struct Vector3 capsuleToHit;
    vector3AddScaled(&ray->origin, &ray->dir, contact->distance, &contact->at);
    vector3Sub(&contact->at, capsulePos, &capsuleToHit);

    float hitHeight = vector3Dot(&capsuleToHit, capsuleUp);
    if (hitHeight > capsule->radius || hitHeight < (-capsule->extendDownward - capsule->radius)) {
        return 0;
    } else if (hitHeight < 0.0f && hitHeight > -capsule->extendDownward) {
        // In range of cylinder
        finishContact(capsuleObject, ray, contact);
        return 1;
    } else {
        // In range of endcaps
        return collisionCapsuleRaycastCap(
            capsuleObject,
            ray,
            hitHeight > 0.0f,  // checkTop
            maxDistance,
            contact
        );
    }
}

struct ColliderCallbacks gCollisionCapsuleCallbacks = {
    collisionCapsuleRaycast,
    collisionCapsuleSolidMofI,
    collisionCapsuleBoundingBox,
    collisionCapsuleMinkowsiSum,
};
