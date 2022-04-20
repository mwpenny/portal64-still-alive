#include "collision_object.h"

void collisionObjectInit(struct CollisionObject* object, struct ColliderTypeData *collider, struct RigidBody* body, float mass) {
    object->collider = collider;
    object->body = body;
    rigidBodyInit(body, mass, collider->callbacks->mofICalculator(collider, mass));
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

    struct ContactConstraintState localContact;
    localContact.contactCount = 0;

    struct ContactConstraintState* contact = contactSolverPeekContact(contactSolver, quad, object);
    
    if (quadCollider(object->collider->data, &object->body->transform, quad->collider->data, &localContact)) {
        if (!contact) {
            return;
        }
        
        contactSolverAssign(contact, &localContact, 1);
    } else if (contact) {
        contactSolverRemoveContact(contactSolver, contact);
    }
}