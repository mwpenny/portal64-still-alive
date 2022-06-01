#include "collision_object.h"

void collisionObjectInit(struct CollisionObject* object, struct ColliderTypeData *collider, struct RigidBody* body, float mass) {
    object->collider = collider;
    object->body = body;
    rigidBodyInit(body, mass, collider->callbacks->mofICalculator(collider, mass));
    collisionObjectUpdateBB(object);
}


void collisionObjectCollideWithPlane(struct CollisionObject* object, struct CollisionObject* plane, struct ContactSolver* contactSolver) {
    CollideWithPlane planeCollider = object->collider->callbacks->collideWithPlane;

    if (!planeCollider) {
        return;
    }

    struct ContactConstraintState localContact;
    localContact.contactCount = 0;

    struct ContactConstraintState* contact = contactSolverPeekContact(contactSolver, plane, object);
    
    if (planeCollider(object->collider->data, &object->body->transform, plane->collider->data, &localContact)) {

        if (!contact) {
            return;
        }
        
        contactSolverAssign(contact, &localContact, 1);
    } else if (contact) {
        contactSolverRemoveContact(contactSolver, contact);
    }
}

void collisionObjectCollideWithQuad(struct CollisionObject* object, struct CollisionObject* quad, struct ContactSolver* contactSolver) {
    CollideWithQuad quadCollider = object->collider->callbacks->collideWithQuad;

    if (!quadCollider) {
        return;
    }

    if (!box3DHasOverlap(&object->boundingBox, &quad->boundingBox)) {
        return;
    }

    struct ContactConstraintState localContact;
    localContact.contactCount = 0;

    struct ContactConstraintState* contact = contactSolverPeekContact(contactSolver, quad, object);
    
    if (quadCollider(object->collider->data, &object->body->transform, quad->collider->data, &localContact)) {
        if (!contact) {
            return;
        }
        
        if (contactSolverAssign(contact, &localContact, 1) && object->body) {
            object->body->flags |= RigidBodyIsTouchingPortal;
        }
    } else if (contact) {
        contactSolverRemoveContact(contactSolver, contact);
    }
}

void collisionObjectUpdateBB(struct CollisionObject* object) {
    if (object->body) {
        object->collider->callbacks->boundingBoxCalculator(object->collider, &object->body->transform, &object->boundingBox);
        basisFromQuat(&object->body->rotationBasis, &object->body->transform.rotation);
    }
}

void minkowsiSumAgainstQuad(void* data, struct Vector3* direction, struct Vector3* output) {
    struct CollisionQuad* quad = (struct CollisionQuad*)data;
    *output = quad->corner;

    if (vector3Dot(&quad->edgeA, direction) > 0.0f) {
        vector3AddScaled(output, &quad->edgeA,  quad->edgeALength, output);
    }

    if (vector3Dot(&quad->edgeB, direction) > 0.0f) {
        vector3AddScaled(output, &quad->edgeB,  quad->edgeBLength, output);
    }
}

// data should be of type struct MinkowsiSumAgainstObjects
void minkowsiSumAgainstObject(void* data, struct Vector3* direction, struct Vector3* output) {
    struct CollisionObject* object = (struct CollisionObject*)data;
    object->collider->callbacks->minkowsiSum(object->collider->data, &object->body->rotationBasis, direction, output);
    vector3Add(output, &object->body->transform.position, output);
}
