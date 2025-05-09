#ifndef __PHYSICS_COMPOUND_COLLIDER_H__
#define __PHYSICS_COMPOUND_COLLIDER_H__

#include "collision_object.h"

struct CompoundCollider {
    // TODO
    struct CollisionObject* children[4];
    short childrenCount;
};

extern struct ColliderCallbacks gCompoundColliderCallbacks;

void compoundColliderCollideObject(struct CollisionObject* compoundColliderObject, struct CollisionObject* other, struct ContactSolver* contactSolver);

#endif
