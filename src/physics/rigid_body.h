#ifndef __RIGID_BODY_H__
#define __RIGID_BODY_H__

#include "../math/transform.h"
#include "collision.h"

struct RigidBody {
    struct ColliderTypeData* collider;

    struct Transform transform;
    struct Vector3 velocity;
    struct Vector3 angularVelocity;

    float mass;
    float massInv;
    // most objects are going to be spheres
    // and cubes. Any other object will just
    // have some inaccurate physics 
    // for cube m * side * side / 6
    // for sphere 2/5 * m * r * r
    float momentOfInertia;
    float momentOfInertiaInv;
};

void rigidBodyInit(struct RigidBody* rigidBody, struct ColliderTypeData* collider, float mass);
void rigidBodyAppyImpulse(struct RigidBody* rigidBody, struct Vector3* worldPoint, struct Vector3* impulse);
void rigidBodyUpdate(struct RigidBody* rigidBody);
void rigidBodyVelocityAtLocalPoint(struct RigidBody* rigidBody, struct Vector3* localPoint, struct Vector3* worldVelocity);
void rigidBodyVelocityAtWorldPoint(struct RigidBody* rigidBody, struct Vector3* worldPoint, struct Vector3* worldVelocity);

void rigidBodyResolveContact(struct RigidBody* bodyA, struct RigidBody* bodyB, struct ContactPoint* contactPoint);

void rigidBodyCollideWithPlane(struct RigidBody* rigidBody, struct Plane* plane, struct ContactSolver* contactSolver);

#endif