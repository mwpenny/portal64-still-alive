#ifndef __RIGID_BODY_H__
#define __RIGID_BODY_H__

#include "../math/basis.h"
#include "../math/transform.h"
#include "./collision.h"

#define RIGID_BODY_NO_ROOM  0xFFFF

#define MAX_PORTAL_SPEED (1000.0f / 64.0f)

enum RigidBodyFlags {
    RigidBodyFlagsInFrontPortal0 = (1 << 0),
    RigidBodyFlagsInFrontPortal1 = (1 << 1),
    RigidBodyFlagsPortalsInactive = (1 << 2),
    RigidBodyFlagsCrossedPortal0 = (1 << 3),
    RigidBodyFlagsCrossedPortal1 = (1 << 4),
    RigidBodyFlagsGrabbable = (1 << 5),
    RigidBodyIsTouchingPortalA = (1 << 6),
    RigidBodyIsTouchingPortalB = (1 << 7),
    RigidBodyWasTouchingPortalA = (1 << 8),
    RigidBodyWasTouchingPortalB = (1 << 9),

    RigidBodyIsKinematic = (1 << 10),
    RigidBodyIsSleeping = (1 << 11),
    // tells the collision system to generate contacts with the player
    RigidBodyIsPlayer = (1 << 12),

    RigidBodyFizzled = (1 << 13),
    RigidBodyDisableGravity = (1 << 14),
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