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
        
        contactSolverAssign(contact, &localContact);
    } else if (contact) {
        contactSolverRemoveContact(contactSolver, contact);
    }
}