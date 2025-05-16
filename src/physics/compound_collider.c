#include "compound_collider.h"

#include "epa.h"
#include "raycasting.h"

int compoundColliderRaycast(struct CollisionObject* object, struct Ray* ray, float maxDistance, struct RaycastHit* contact) {
    float distance = rayDetermineDistance(ray, &object->body->transform.position);

    if (distance < 0.0f) {
        return 0;
    }

    struct Vector3 nearestPoint;
    vector3AddScaled(&ray->origin, &ray->dir, distance, &nearestPoint);

    struct Vector3 boundingBoxSideLengths;
    vector3Sub(&object->boundingBox.max, &object->boundingBox.min, &boundingBoxSideLengths);
    vector3Scale(&boundingBoxSideLengths, &boundingBoxSideLengths, 0.5f);

    if (vector3DistSqrd(&object->body->transform.position, &nearestPoint) > vector3MagSqrd(&boundingBoxSideLengths)) {
        return 0;
    }

    struct CompoundCollider* collider = (struct CompoundCollider*)object->collider->data;

    int found = 0;

    for (short i = 0; i < collider->childrenCount; ++i) {
        struct CollisionObject* childObj = collider->children[i];
        struct ColliderTypeData* childCollider = childObj->collider;

        if (childCollider->callbacks->raycast(childObj, ray, maxDistance, contact)) {
            maxDistance = contact->distance;
            found = 1;
        }
    }

    if (found) {
        // Return compound collider, not child
        contact->object = object;
    }

    return found;
}

float compoundColliderSolidMofI(struct ColliderTypeData* typeData, float mass) {
    struct CompoundCollider* collider = (struct CompoundCollider*)typeData->data;

    float childMass = mass / collider->childrenCount;
    float mofi = 0.0f;

    for (short i = 0; i < collider->childrenCount; ++i) {
        struct CollisionObject* childObj = collider->children[i];
        struct ColliderTypeData* childCollider = childObj->collider;

        mofi += childCollider->callbacks->mofICalculator(childCollider, childMass);
    }

    return mofi;
}

void compoundColliderBoundingBox(struct ColliderTypeData* typeData, struct Transform* transform, struct Box3D* box) {
    struct CompoundCollider* collider = (struct CompoundCollider*)typeData->data;

    for (short i = 0; i < collider->childrenCount; ++i) {
        struct CollisionObject* childObj = collider->children[i];
        struct ColliderTypeData* childCollider = childObj->collider;

        struct Transform* childTransform = transform;

        struct Transform offsetTransform;
        if (childObj->bodyOffset) {
            offsetTransform = *transform;

            struct Vector3 offset;
            quatMultVector(&offsetTransform.rotation, childObj->bodyOffset, &offset);
            vector3Add(&offsetTransform.position, &offset, &offsetTransform.position);
            childTransform = &offsetTransform;
        }

        childCollider->callbacks->boundingBoxCalculator(childCollider, childTransform, &childObj->boundingBox);

        if (i == 0) {
            *box = childObj->boundingBox;
        } else {
            box3DUnion(box, &childObj->boundingBox, box);
        }
    }
}

void compoundColliderCollideObject(struct CollisionObject* compoundColliderObject, struct CollisionObject* other, struct ContactSolver* contactSolver) {
    struct CompoundCollider* compoundCollider = (struct CompoundCollider*)compoundColliderObject->collider->data;

    for (short i = 0; i < compoundCollider->childrenCount; ++i) {
        struct CollisionObject* childObj = compoundCollider->children[i];
        collisionObjectCollideTwoObjects(childObj, other, contactSolver);
    }
}

void compoundColliderCollideObjectSwept(
    struct CollisionObject* compoundColliderObject,
    struct Vector3* prevCompoundColliderPos,
    struct Box3D* sweptCompoundCollider,
    struct CollisionObject* other,
    struct Vector3* prevOtherPos,
    struct Box3D* sweptOther,
    struct ContactSolver* contactSolver
) {
    struct CompoundCollider* compoundCollider = (struct CompoundCollider*)compoundColliderObject->collider->data;

    for (short i = 0; i < compoundCollider->childrenCount; ++i) {
        struct CollisionObject* childObj = compoundCollider->children[i];

        struct Vector3 childPrevPos;
        quatMultVector(&compoundColliderObject->body->transform.rotation, childObj->bodyOffset, &childPrevPos);
        vector3Add(prevCompoundColliderPos, &childPrevPos, &childPrevPos);

        collisionObjectCollideTwoObjectsSwept(
            childObj, &childPrevPos, sweptCompoundCollider,
            other, prevOtherPos, sweptOther,
            contactSolver
        );
    }
}

struct ColliderCallbacks gCompoundColliderCallbacks = {
    compoundColliderRaycast,
    compoundColliderSolidMofI,
    compoundColliderBoundingBox,
    NULL,
};
