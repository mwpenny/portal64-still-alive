#ifndef __RIGID_BODY_H__
#define __RIGID_BODY_H__

#include "../math/transform.h"
#include "collision.h"

enum RigidBodyFlags {
    RigidBodyFlagsInFrontPortal0 = (1 << 0),
    RigidBodyFlagsInFrontPortal1 = (1 << 1),
    RigidBodyFlagsPortalsInactive = (1 << 2),
    RigidBodyFlagsCrossedPortal0 = (1 << 3),
    RigidBodyFlagsCrossedPortal1 = (1 << 4),
    RigidBodyFlagsGrabbable = (1 << 5),
};

struct RigidBody {
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
    
    enum RigidBodyFlags flags;
};

void rigidBodyInit(struct RigidBody* rigidBody, float mass, float momentOfIniteria);
void rigidBodyAppyImpulse(struct RigidBody* rigidBody, struct Vector3* worldPoint, struct Vector3* impulse);
void rigidBodyUpdate(struct RigidBody* rigidBody);
void rigidBodyVelocityAtLocalPoint(struct RigidBody* rigidBody, struct Vector3* localPoint, struct Vector3* worldVelocity);
void rigidBodyVelocityAtWorldPoint(struct RigidBody* rigidBody, struct Vector3* worldPoint, struct Vector3* worldVelocity);

void rigidBodyCheckPortals(struct RigidBody* rigidBody);


#endif