#ifndef __PHYSICS_COMPOUND_COLLIDER_H__
#define __PHYSICS_COMPOUND_COLLIDER_H__

#include "collision_object.h"

struct CompoundCollider {
    // TODO
    struct CollisionObject* children[4];
    short childrenCount;
};

extern struct ColliderCallbacks gCompoundColliderCallbacks;

void compoundColliderCollideObject(
    struct CollisionObject* compoundColliderObject,
    struct CollisionObject* other,
    struct ContactSolver* contactSolver
);

void compoundColliderCollideObjectSwept(
    struct CollisionObject* compoundColliderObject,
    struct Vector3* prevCompoundColliderPos,
    struct Box3D* sweptCompoundCollider,
    struct CollisionObject* other,
    struct Vector3* prevOtherPos,
    struct Box3D* sweptOther,
    struct ContactSolver* contactSolver
);

#endif
