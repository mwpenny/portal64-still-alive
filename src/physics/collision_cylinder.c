#include "collision_cylinder.h"

#include "../math/mathf.h"
#include "contact_solver.h"
#include "collision_quad.h"
#include "raycasting.h"
#include "line.h"
#include "../math/vector2.h"
#include "./raycasting.h"

struct ColliderCallbacks gCollisionCylinderCallbacks = {
    collisionCylinderRaycast,
    collisionCylinderSolidMofI,
    collisionCylinderBoundingBox,
    collisionCylinderMinkowsiSum,
};

void collisionCylinderBoxCheckForFaces(struct CollisionCylinder* cylinder) {
    if (cylinder->outsideFaces[0].edgeALength > 0.0f) {
        return;
    }

    struct Vector2 prevPoint;
    vector2Scale(&cylinder->edgeVectors[cylinder->edgeCount - 1], -cylinder->radius, &prevPoint);

    for (int i = 0; i < (cylinder->edgeCount << 1); ++i) {
        struct Vector2 currPoint;
        vector2Scale(&cylinder->edgeVectors[i % cylinder->edgeCount], i >= cylinder->edgeCount ? -cylinder->radius : cylinder->radius, &currPoint);

        struct CollisionQuad* quad = &cylinder->outsideFaces[i];

        quad->corner.x = prevPoint.x;
        quad->corner.y = -cylinder->halfHeight;
        quad->corner.z = prevPoint.y;

        struct Vector3 toEdge;

        toEdge.x = currPoint.x;
        toEdge.y = -cylinder->halfHeight;
        toEdge.z = currPoint.y;

        vector3Sub(&toEdge, &quad->corner, &quad->edgeA);
        quad->edgeALength = sqrtf(vector3MagSqrd(&quad->edgeA));
        vector3Scale(&quad->edgeA, &quad->edgeA, 1.0f / quad->edgeALength);

        quad->edgeB = gUp;
        quad->edgeBLength = cylinder->halfHeight * 2.0f;

        vector3Cross(&quad->edgeA, &quad->edgeB, &quad->plane.normal);

        if (vector3Dot(&quad->plane.normal, &quad->corner) < 0.0f) {
            struct Vector3 tmpEdge = quad->edgeA;
            quad->edgeA = quad->edgeB;
            quad->edgeB = tmpEdge;

            float tmpLen = quad->edgeALength;
            quad->edgeALength = quad->edgeBLength;
            quad->edgeBLength = tmpLen;

            vector3Negate(&quad->plane.normal, &quad->plane.normal);
        }

        quad->plane.d = -vector3Dot(&quad->plane.normal, &quad->corner);

        prevPoint = currPoint;
    }
}

float collisionCylinderSolidMofI(struct ColliderTypeData* typeData, float mass) {
    struct CollisionCylinder* cylinder = (struct CollisionCylinder*)typeData->data;

    float rr = cylinder->radius * cylinder->radius;

    float topResult = 0.5f * mass * rr;
    float sideResult = (1.0f / 12.0f) * mass * (3.0f * rr + 4.0f * cylinder->halfHeight * cylinder->halfHeight);

    return MAX(topResult, sideResult);
}

void collisionCylinderBoundingBox(struct ColliderTypeData* typeData, struct Transform* transform, struct Box3D* box) {
    struct CollisionCylinder* cylinder = (struct CollisionCylinder*)typeData->data;
    struct Vector3 radius;
    radius.x = cylinder->radius;
    radius.y = cylinder->halfHeight;
    radius.z = cylinder->radius;
    struct Vector3 halfSize;
    quatRotatedBoundingBoxSize(&transform->rotation, &radius, &halfSize);
    vector3Sub(&transform->position, &halfSize, &box->min);
    vector3Add(&transform->position, &halfSize, &box->max);
}

int collisionCylinderMinkowsiSum(void* data, struct Basis* basis, struct Vector3* direction, struct Vector3* output) {
    struct CollisionCylinder* cylinder = (struct CollisionCylinder*)data;
    int centerDir = vector3Dot(&basis->y, direction) > 0.0f;

    vector3Scale(&basis->y, output, centerDir > 0.0f ? cylinder->halfHeight : -cylinder->halfHeight);

    struct Vector2 crossDirectionCheck;
    crossDirectionCheck.x = vector3Dot(&basis->x, direction);
    crossDirectionCheck.y = vector3Dot(&basis->z, direction);


    float dotDiff = vector2Dot(&crossDirectionCheck, &cylinder->edgeVectors[0]);
    int closest = 0;

    for (int i = 1; i < cylinder->edgeCount; ++i) {
        float dotCheck = vector2Dot(&crossDirectionCheck, &cylinder->edgeVectors[i]);

        if (fabsf(dotCheck) > fabsf(dotDiff)) {
            dotDiff = dotCheck;
            closest = i;
        }
    }

    struct Vector2 crossDirection;

    int faceId = closest;

    if (dotDiff < 0.0f) {
        faceId += cylinder->edgeCount;
        vector2Negate(&cylinder->edgeVectors[closest], &crossDirection);
    } else {
        crossDirection = cylinder->edgeVectors[closest];
    }

    int nextId = faceId + 1;

    if (faceId == cylinder->edgeCount * 2) {
        nextId = 0;
    }

    vector3AddScaled(output, &basis->x, crossDirection.x * cylinder->radius, output);
    vector3AddScaled(output, &basis->z, crossDirection.y * cylinder->radius, output);

    return (centerDir ? 0x1 : 0x2) | (1 << (faceId + 2)) | (1 << (nextId + 2));
}

int collisionCylinderRaycastCap(struct CollisionObject* cylinderObject, struct Ray* ray, struct Ray* localRay, float maxDistance, struct RaycastHit* contact) {
    struct CollisionCylinder* cylinder = (struct CollisionCylinder*)cylinderObject->collider->data;

    struct Plane localPlane;

    localPlane.normal = gZeroVec;
    localPlane.normal.y = localRay->dir.y > 0.0f ? -1.0f : 1.0f;
    localPlane.d = -cylinder->halfHeight;

    float capHitDistance;
    if (!planeRayIntersection(&localPlane, &localRay->origin, &localRay->dir, &capHitDistance)) {
        return 0;
    }

    if (capHitDistance < 0.0f || capHitDistance > maxDistance) {
        return 0;
    }
    
    struct Vector3 localHit;
    vector3AddScaled(&localRay->origin, &localRay->dir, capHitDistance, &localHit);

    for (int i = 0; i < (cylinder->edgeCount << 1); ++i) {
        if (planePointDistance(&cylinder->outsideFaces[i].plane, &localHit) > 0.0f) {
            return 0;
        }
    }

    vector3AddScaled(&ray->origin, &ray->dir, capHitDistance, &contact->at);
    if (localPlane.normal.y > 0.0f) {
        contact->normal = cylinderObject->body->rotationBasis.y;
    } else {
        vector3Negate(&cylinderObject->body->rotationBasis.y, &contact->normal);
    }

    contact->distance = capHitDistance;
    contact->object = cylinderObject;
    contact->throughPortal = 0;
    contact->roomIndex = cylinderObject->body->currentRoom;

    return 1;
}

int collisionCylinderRaycast(struct CollisionObject* cylinderObject, struct Ray* ray, float maxDistance, struct RaycastHit* contact) {
    float rayLerp;
    float cylinderLerp;

    struct CollisionCylinder* cylinder = (struct CollisionCylinder*)cylinderObject->collider->data;
    collisionCylinderBoxCheckForFaces(cylinder);

    if (!lineNearestApproach(&ray->origin, &ray->dir, &cylinderObject->body->transform.position, &cylinderObject->body->rotationBasis.y, &rayLerp, &cylinderLerp)) {
        struct Ray localRay;
        collisionObjectLocalRay(cylinderObject, ray, &localRay);
        return collisionCylinderRaycastCap(cylinderObject, ray, &localRay, maxDistance, contact);
    }

    if (rayLerp < 0.0f) {
        return 0;
    }

    float diagnalReach = cylinder->radius * cylinder->radius + cylinder->halfHeight * cylinder->halfHeight;

    float distanceCheck = maxDistance + diagnalReach;

    if (rayLerp > distanceCheck * distanceCheck) {
        return 0;
    }

    struct Vector3 rayApproach;
    vector3AddScaled(&ray->origin, &ray->dir, rayLerp, &rayApproach);

    struct Vector3 cylinderApproach;
    vector3AddScaled(&cylinderObject->body->transform.position, &cylinderObject->body->rotationBasis.y, cylinderLerp, &cylinderApproach);

    if (vector3DistSqrd(&rayApproach, &cylinderApproach) > cylinder->radius * cylinder->radius) {
        return 0;
    }

    struct Ray localRay;
    collisionObjectLocalRay(cylinderObject, ray, &localRay);

    for (int i = 0; i < (cylinder->edgeCount << 1); ++i) {
        if (raycastQuadShape(&cylinder->outsideFaces[i], &localRay, maxDistance, contact)) {
            struct Vector3 rotatedNormal;
            basisRotate(&cylinderObject->body->rotationBasis, &contact->normal, &rotatedNormal);
            contact->normal = rotatedNormal;
            vector3AddScaled(&ray->origin, &ray->dir, contact->distance, &contact->at);

            contact->object = cylinderObject;
            contact->throughPortal = 0;
            contact->roomIndex = cylinderObject->body->currentRoom;
            
            return 1;
        }
    }

    return collisionCylinderRaycastCap(cylinderObject, ray, &localRay, maxDistance, contact);
}