#ifndef __RIGID_BODY_H__
#define __RIGID_BODY_H__

#include "../math/transform.h"
#include "collision.h"

#define MAX_CONTACT_POINT_COUNT 8

struct RigidBody {
    enum CollisionShapeType collisionType;
    void* collisionShape;
    struct ContactPoint gContactList[MAX_CONTACT_POINT_COUNT];

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

#endif