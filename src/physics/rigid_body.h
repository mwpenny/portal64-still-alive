#ifndef __RIGID_BODY_H__
#define __RIGID_BODY_H__

#include "../math/basis.h"
#include "../math/transform.h"
#include "./collision.h"

#define RIGID_BODY_NO_ROOM  0xFFFF

enum RigidBodyFlags {
    RigidBodyFlagsInFrontPortal0 = (1 << 0),
    RigidBodyFlagsInFrontPortal1 = (1 << 1),
    RigidBodyFlagsPortalsInactive = (1 << 2),
    RigidBodyFlagsCrossedPortal0 = (1 << 3),
    RigidBodyFlagsCrossedPortal1 = (1 << 4),
    RigidBodyFlagsGrabbable = (1 << 5),
    RigidBodyIsTouchingPortal = (1 << 6),
    RigidBodyWasTouchingPortal = (1 << 7),

    RigidBodyIsKinematic = (1 << 8),
    RigidBodyIsSleeping = (1 << 9),
    // for kinematic bodies that should generate 
    // contacts with other kinematic bodies
    RigidBodyGenerateContacts = (1 << 10),

    RigidBodyFizzled = (1 << 11),
    RigidBodyDisableGravity = (1 << 12),
};

struct RigidBody {
    struct Transform transform;
    struct Vector3 velocity;
    struct Vector3 angularVelocity;
    // used to speed up caluclations
    struct Basis rotationBasis;

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
    unsigned short currentRoom;
};

void rigidBodyInit(struct RigidBody* rigidBody, float mass, float momentOfIniteria);
void rigidBodyMarkKinematic(struct RigidBody* rigidBody);
void rigidBodyAppyImpulse(struct RigidBody* rigidBody, struct Vector3* worldPoint, struct Vector3* impulse);
void rigidBodyUpdate(struct RigidBody* rigidBody);
void rigidBodyVelocityAtLocalPoint(struct RigidBody* rigidBody, struct Vector3* localPoint, struct Vector3* worldVelocity);
void rigidBodyVelocityAtWorldPoint(struct RigidBody* rigidBody, struct Vector3* worldPoint, struct Vector3* worldVelocity);
void rigidBodyTeleport(struct RigidBody* rigidBody, struct Transform* from, struct Transform* to, int toRoom);

int rigidBodyCheckPortals(struct RigidBody* rigidBody);


#endif