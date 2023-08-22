#include "mesh_collider.h"

#include "epa.h"
#include "gjk.h"
#include "contact_insertion.h"
#include "raycasting.h"

float meshColliderMofI(struct ColliderTypeData* typeData, float mass);
void meshColliderBoundingBox(struct ColliderTypeData* typeData, struct Transform* transform, struct Box3D* box);

struct ColliderCallbacks gMeshColliderCallbacks = {
    meshColliderRaycast,
    meshColliderMofI,
    meshColliderBoundingBox,
    NULL,
};

struct CollisionObjectWithTransform {
    struct CollisionObject* object;
    struct Transform relativeTransform;
    struct Basis relativeBasis;
};

int minkowsiSumAgainstRelativeObject(void* data, struct Vector3* direction, struct Vector3* output) {
    struct CollisionObjectWithTransform* relativeObject = (struct CollisionObjectWithTransform*)data;
    int result = relativeObject->object->collider->callbacks->minkowsiSum(relativeObject->object->collider->data, &relativeObject->relativeBasis, direction, output);
    vector3Add(output, &relativeObject->relativeTransform.position, output);
    return result;
}

int meshColliderCollideObjectWithSingleQuad(struct CollisionObject* quadObject, struct CollisionObjectWithTransform* other, struct EpaResult* result) {
    if ((quadObject->collisionLayers & other->object->collisionLayers) == 0) {
        return 0;
    }

    struct Simplex simplex;

    struct CollisionQuad* quad = (struct CollisionQuad*)quadObject->collider->data;

    if (!gjkCheckForOverlap(&simplex, 
                quad, minkowsiSumAgainstQuad, 
                other, minkowsiSumAgainstRelativeObject, 
                &quad->plane.normal)) {
        return 0;
    }

    epaSolve(
        &simplex,
        quad, minkowsiSumAgainstQuad,
        other, minkowsiSumAgainstRelativeObject,
        result
    );

    return 1;
}

void meshColliderCollideObject(struct CollisionObject* meshColliderObject, struct CollisionObject* other, struct ContactSolver* contactSolver) {
    struct Transform meshInverse;
    transformInvert(&meshColliderObject->body->transform, &meshInverse);

    struct CollisionObjectWithTransform relativeObject;
    relativeObject.object = other;
    transformConcat(&meshInverse, &other->body->transform, &relativeObject.relativeTransform);
    basisFromQuat(&relativeObject.relativeBasis, &relativeObject.relativeTransform.rotation);

    struct MeshCollider* meshCollider = (struct MeshCollider*)meshColliderObject->collider->data;

    for (int i = 0; i < meshCollider->childrenCount; ++i) {
        struct EpaResult result;

        struct CollisionObject* quadObject = &meshCollider->children[i];

        if (!meshColliderCollideObjectWithSingleQuad(quadObject, &relativeObject, &result)) {
            continue;
        }

        struct ContactManifold* contact = contactSolverGetContactManifold(contactSolver, quadObject, other);

        if (!contact) {
            continue;
        }

        // contactA should be in world coordinates
        transformPoint(&meshColliderObject->body->transform, &result.contactA, &result.contactA);

        // transform contactB to be in the localspace of the other object
        transformPoint(&meshColliderObject->body->transform, &result.contactB, &result.contactB);
        transformPointInverseNoScale(&other->body->transform, &result.contactB, &result.contactB);

        struct Vector3 transformedNormal;
        basisRotate(&meshColliderObject->body->rotationBasis, &result.normal, &transformedNormal);
        result.normal = transformedNormal;

        if (contact->shapeB == quadObject) {
            epaSwapResult(&result);
        }

        contact->friction = MAX(quadObject->collider->friction, other->collider->friction);
        contact->restitution = MIN(quadObject->collider->bounce, other->collider->bounce);

        contactInsert(contact, &result);
    }
}

int meshColliderRaycast(struct CollisionObject* object, struct Ray* ray, float maxDistance, struct RaycastHit* contact) {
    struct MeshCollider* meshCollider = (struct MeshCollider*)object->collider->data;
    struct Ray localRay;
    struct Vector3 rayOffset;
    vector3Sub(&ray->origin, &object->body->transform.position, &rayOffset);
    basisUnRotate(&object->body->rotationBasis, &rayOffset, &localRay.origin);
    basisUnRotate(&object->body->rotationBasis, &ray->dir, &localRay.dir);


    float passDistance = rayDetermineDistance(&localRay, &meshCollider->localCenter);

    // filter out rays that have no chance of hitting
    if (passDistance < -meshCollider->radiusFromCenter || passDistance - meshCollider->radiusFromCenter > maxDistance) {
        return 0;
    }

    struct Vector3 nearestPoint;

    vector3AddScaled(&localRay.origin, &localRay.dir, passDistance, &nearestPoint);

    if (vector3DistSqrd(&nearestPoint, &meshCollider->localCenter) > meshCollider->radiusFromCenter * meshCollider->radiusFromCenter) {
        return 0;
    }
    
    float originalDistance = maxDistance;

    for (int i = 0; i < meshCollider->childrenCount; ++i) {
        struct RaycastHit localHit;
        if (raycastQuad(&meshCollider->children[i], &localRay, maxDistance, &localHit) && localHit.distance < maxDistance) {
            maxDistance = localHit.distance;
            *contact = localHit;
        }
    }

    if (originalDistance == maxDistance) {
        return 0;
    }

    transformPoint(&object->body->transform, &contact->at, &contact->at);
    struct Vector3 rotatedNormal;
    basisRotate(&object->body->rotationBasis, &contact->normal, &rotatedNormal);
    contact->normal = rotatedNormal;

    return 1;
}

// mesh collider should be kinematic 
float meshColliderMofI(struct ColliderTypeData* typeData, float mass) {
    return 1.0f;
}

void meshColliderBoundingBox(struct ColliderTypeData* typeData, struct Transform* transform, struct Box3D* box) {
    struct MeshCollider* collisionMesh = (struct MeshCollider*)typeData->data;
    struct Vector3 halfSize;
    quatRotatedBoundingBoxSize(&transform->rotation, &collisionMesh->localHalfBoundingbox, &halfSize);
    struct Vector3 center;
    transformPoint(transform, &collisionMesh->localCenter, &center);
    vector3Add(&center, &halfSize, &box->max);
    vector3Sub(&center, &halfSize, &box->min);
}