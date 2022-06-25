#include "collision_object.h"
#include "epa.h"
#include "gjk.h"
#include "contact_insertion.h"
#include "collision_scene.h"
#include "../math/mathf.h"
#include "mesh_collider.h"

void collisionObjectInit(struct CollisionObject* object, struct ColliderTypeData *collider, struct RigidBody* body, float mass, int collisionLayers) {
    object->collider = collider;
    object->body = body;
    rigidBodyInit(body, mass, collider->callbacks->mofICalculator(collider, mass));
    collisionObjectUpdateBB(object);
    object->collisionLayers = collisionLayers;
    object->data = 0;
    object->trigger = 0;
}

int collisionObjectIsActive(struct CollisionObject* object) {
    return object->body && (object->body->flags & (RigidBodyIsKinematic | RigidBodyIsSleeping)) == 0;
}

int collisionObjectShouldGenerateConctacts(struct CollisionObject* object) {
    return collisionObjectIsActive(object) || (object->body->flags & RigidBodyGenerateContacts) != 0;
}

void collisionObjectCollideWithQuad(struct CollisionObject* object, struct CollisionObject* quadObject, struct ContactSolver* contactSolver) {
    if ((object->collisionLayers & quadObject->collisionLayers) == 0) {
        return;
    }

    if (!box3DHasOverlap(&object->boundingBox, &quadObject->boundingBox)) {
        return;
    }

    if (object->trigger && quadObject->trigger) {
        return;
    }

    struct Simplex simplex;

    struct CollisionQuad* quad = (struct CollisionQuad*)quadObject->collider->data;

    if (!gjkCheckForOverlap(&simplex, 
                quad, minkowsiSumAgainstQuad, 
                object, minkowsiSumAgainstObject, 
                &quad->plane.normal)) {
        return;
    }

    if (object->trigger) {
        object->trigger(object->data, quadObject);
        return;
    }

    if (quadObject->trigger) {
        quadObject->trigger(quadObject->data, object);
        return;
    }

    struct EpaResult result;
    epaSolve(
        &simplex, 
        quad, minkowsiSumAgainstQuad, 
        object, minkowsiSumAgainstObject, 
        &result
    );

    if (collisionSceneIsTouchingPortal(&result.contactA, &result.normal)) {
        object->body->flags |= RigidBodyIsTouchingPortal;
        return;
    }

    struct ContactManifold* contact = contactSolverGetContactManifold(contactSolver, quadObject, object);

    if (!contact) {
        return;
    }

    contact->friction = MAX(object->collider->friction, quadObject->collider->friction);
    contact->restitution = MIN(object->collider->bounce, quadObject->collider->bounce);

    transformPointInverseNoScale(&object->body->transform, &result.contactB, &result.contactB);
    contactInsert(contact, &result);
}


void collisionObjectCollideTwoObjects(struct CollisionObject* a, struct CollisionObject* b, struct ContactSolver* contactSolver) {
    if (!box3DHasOverlap(&a->boundingBox, &b->boundingBox)) {
        return;
    }

    if (a->trigger && b->trigger) {
        return;
    }

    if (a->collider->type == CollisionShapeTypeMesh) {
        if (b->collider->type != CollisionShapeTypeMesh) {
            meshColliderCollideObject(a, b, contactSolver);
        }

        return;
    } else if (b->collider->type == CollisionShapeTypeMesh) {
        meshColliderCollideObject(b, a, contactSolver);
        return;
    }

    struct Simplex simplex;

    struct Vector3 offset;

    vector3Sub(&b->body->transform.position, &a->body->transform.position, &offset);

    if (!gjkCheckForOverlap(&simplex, 
                a, minkowsiSumAgainstObject, 
                b, minkowsiSumAgainstObject, 
                &offset)) {
        return;
    }

    if (a->trigger) {
        a->trigger(a->data, b);
        return;
    }

    if (b->trigger) {
        b->trigger(b->data, a);
        return;
    }

    struct EpaResult result;
    epaSolve(
        &simplex, 
        a, minkowsiSumAgainstObject, 
        b, minkowsiSumAgainstObject, 
        &result
    );

    struct ContactManifold* contact = contactSolverGetContactManifold(contactSolver, a, b);

    if (!contact) {
        return;
    }

    transformPointInverseNoScale(&a->body->transform, &result.contactA, &result.contactA);
    transformPointInverseNoScale(&b->body->transform, &result.contactB, &result.contactB);

    if (contact->shapeA == b) {
        epaSwapResult(&result);
    }

    contact->friction = MAX(a->collider->friction, b->collider->friction);
    contact->restitution = MIN(a->collider->bounce, b->collider->bounce);

    contactInsert(contact, &result);
}

void collisionObjectUpdateBB(struct CollisionObject* object) {
    if (object->body) {
        object->collider->callbacks->boundingBoxCalculator(object->collider, &object->body->transform, &object->boundingBox);
        basisFromQuat(&object->body->rotationBasis, &object->body->transform.rotation);
    }
}

int minkowsiSumAgainstQuad(void* data, struct Vector3* direction, struct Vector3* output) {
    struct CollisionQuad* quad = (struct CollisionQuad*)data;
    *output = quad->corner;

    int result = 0;

    if (vector3Dot(&quad->edgeA, direction) > 0.0f) {
        vector3AddScaled(output, &quad->edgeA, quad->edgeALength, output);
        result |= 0x1;
    } else {
        result |= 0x2;
    }

    if (vector3Dot(&quad->edgeB, direction) > 0.0f) {
        vector3AddScaled(output, &quad->edgeB, quad->edgeBLength, output);
        result |= 0x4;
    } else {
        result |= 0x8;
    }

    if (quad->thickness > 0.0f) {
        if (vector3Dot(&quad->plane.normal, direction) < 0.0f) {
            vector3AddScaled(output, &quad->plane.normal, -quad->thickness, output);
            result |= 0x10;
        } else {
            result |= 0x20;
        }
    } else {
        result |= 0x20;
    }

    return result;
}

int minkowsiSumAgainstObject(void* data, struct Vector3* direction, struct Vector3* output) {
    struct CollisionObject* object = (struct CollisionObject*)data;
    int result = object->collider->callbacks->minkowsiSum(object->collider->data, &object->body->rotationBasis, direction, output);
    vector3Add(output, &object->body->transform.position, output);
    return result;
}

void collisionObjectLocalRay(struct CollisionObject* cylinderObject, struct Ray* ray, struct Ray* localRay) {
    struct Vector3 offset;
    vector3Sub(&ray->origin, &cylinderObject->body->transform.position, &offset);
    basisUnRotate(&cylinderObject->body->rotationBasis, &ray->dir, &localRay->dir);
    basisUnRotate(&cylinderObject->body->rotationBasis, &offset, &localRay->origin);
}