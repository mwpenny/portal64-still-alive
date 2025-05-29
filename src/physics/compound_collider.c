#include "compound_collider.h"

#include <assert.h>

#include "collision_scene.h"
#include "epa.h"
#include "raycasting.h"
#include "util/memory.h"

void compoundColliderInit(
    struct CompoundCollider* collider,
    struct CompoundColliderDefinition* definition,
    struct RigidBody* body,
    int collisionLayers
) {
    assert(definition->childrenCount > 0);
    assert(definition->childrenCount <= COMPOUND_COLLIDER_MAX_CHILD_COUNT);

    for (short i = 0; i < definition->childrenCount; ++i) {
        struct CompoundColliderComponentDefinition* childDefinition = &definition->childDefinitions[i];
        struct CompoundColliderComponent* child = &collider->children[i];
        struct CollisionObject* childObj = &child->object;

        child->parentOffset = &childDefinition->parentOffset;

        zeroMemory(childObj, sizeof(*childObj));
        childObj->collider = &childDefinition->collider;
        childObj->body = body;
        childObj->position = &child->position;
        childObj->collisionLayers = collisionLayers;
    }

    collider->childrenCount = definition->childrenCount;

    collider->colliderType.type = CollisionShapeTypeCompound;
    collider->colliderType.data = collider;
    collider->colliderType.callbacks = &gCompoundColliderCallbacks;
}

int compoundColliderRaycast(struct CollisionObject* object, struct Ray* ray, float maxDistance, struct RaycastHit* contact) {
    float distance = rayDetermineDistance(ray, object->position);

    if (distance < 0.0f) {
        return 0;
    }

    struct Vector3 nearestPoint;
    vector3AddScaled(&ray->origin, &ray->dir, distance, &nearestPoint);

    struct Vector3 boundingBoxSideLengths;
    vector3Sub(&object->boundingBox.max, &object->boundingBox.min, &boundingBoxSideLengths);
    vector3Scale(&boundingBoxSideLengths, &boundingBoxSideLengths, 0.5f);

    if (vector3DistSqrd(object->position, &nearestPoint) > vector3MagSqrd(&boundingBoxSideLengths)) {
        return 0;
    }

    struct CompoundCollider* collider = (struct CompoundCollider*)object->collider->data;

    int found = 0;

    for (short i = 0; i < collider->childrenCount; ++i) {
        struct CollisionObject* childObj = &collider->children[i].object;
        struct ColliderTypeData* childCollider = childObj->collider;

        if (childCollider->callbacks->raycast &&
            childCollider->callbacks->raycast(childObj, ray, maxDistance, contact)) {
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
        struct CollisionObject* childObj = &collider->children[i].object;
        struct ColliderTypeData* childCollider = childObj->collider;

        mofi += childCollider->callbacks->mofICalculator(childCollider, childMass);
    }

    return mofi;
}

void compoundColliderBoundingBox(struct ColliderTypeData* typeData, struct Transform* transform, struct Box3D* box) {
    struct CompoundCollider* collider = (struct CompoundCollider*)typeData->data;

    for (short i = 0; i < collider->childrenCount; ++i) {
        struct CompoundColliderComponent* child = &collider->children[i];
        struct CollisionObject* childObj = &child->object;
        struct ColliderTypeData* childCollider = childObj->collider;

        quatMultVector(&transform->rotation, child->parentOffset, &child->position);
        vector3Add(&child->position, &transform->position, &child->position);

        struct Transform childTransform;
        childTransform.position = child->position;
        childTransform.rotation = transform->rotation;
        childTransform.scale = transform->scale;

        childCollider->callbacks->boundingBoxCalculator(childCollider, &childTransform, &childObj->boundingBox);

        if (i == 0) {
            *box = childObj->boundingBox;
        } else {
            box3DUnion(box, &childObj->boundingBox, box);
        }
    }
}

void compoundColliderFurthestPoint(
    struct CollisionObject* compoundColliderObject,
    struct Vector3* direction,
    struct Vector3* point
) {
    struct CompoundCollider* compoundCollider = (struct CompoundCollider*)compoundColliderObject->collider->data;

    float maxDistance = 0.0f;

    for (short i = 0; i < compoundCollider->childrenCount; ++i) {
        struct CollisionObject* childObj = &compoundCollider->children[i].object;

        struct Vector3 currentPoint;
        objectMinkowskiSupport(childObj, direction, &currentPoint);

        float distance = vector3Dot(direction, &currentPoint);
        if (distance > maxDistance) {
            maxDistance = distance;
            *point = currentPoint;
        }
    }
}

int compoundColliderHasOverlap(
    struct CollisionObject* compoundColliderObject,
    void* other,
    MinkowskiSupport otherSupport,
    struct Vector3* firstDirection
) {
    struct CompoundCollider* compoundCollider = (struct CompoundCollider*)compoundColliderObject->collider->data;

    for (short i = 0; i < compoundCollider->childrenCount; ++i) {
        struct CollisionObject* childObj = &compoundCollider->children[i].object;

        struct Simplex simplex;
        if (gjkCheckForOverlap(&simplex,
            childObj, objectMinkowskiSupport,
            other, otherSupport,
            firstDirection
        )) {
            return 1;
        }
    }

    return 0;
}

void compoundColliderCollideMixed(
    struct CollisionObject* compoundColliderObject,
    struct Vector3* prevCompoundColliderPos,
    struct Box3D* sweptCompoundCollider,
    struct CollisionScene* scene,
    struct ContactSolver* contactSolver
) {
    struct CompoundCollider* compoundCollider = (struct CompoundCollider*)compoundColliderObject->collider->data;

    for (short i = 0; i < compoundCollider->childrenCount; ++i) {
        struct CollisionObject* childObj = &compoundCollider->children[i].object;

        collisionObjectCollideMixed(
            childObj, prevCompoundColliderPos, sweptCompoundCollider,
            scene, contactSolver
        );
    }
}

void compoundColliderCollidePairMixed(
    struct CollisionObject* compoundColliderObject,
    struct Vector3* prevCompoundColliderPos,
    struct Box3D* sweptCompoundCollider,
    struct CollisionObject* other,
    struct Vector3* prevOtherPos,
    struct Box3D* sweptOther,
    struct ContactSolver* contactSolver
) {
    struct CompoundCollider* compoundCollider = (struct CompoundCollider*)compoundColliderObject->collider->data;

    if (!box3DHasOverlap(sweptCompoundCollider, sweptOther)) {
        return;
    }

    for (short i = 0; i < compoundCollider->childrenCount; ++i) {
        struct CollisionObject* childObj = &compoundCollider->children[i].object;

        collisionObjectCollidePairMixed(
            childObj, prevCompoundColliderPos, sweptCompoundCollider,
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
