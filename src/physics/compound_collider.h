#ifndef __PHYSICS_COMPOUND_COLLIDER_H__
#define __PHYSICS_COMPOUND_COLLIDER_H__

#include "collision_object.h"

struct CompoundCollider {
    struct CollisionObject* children;
    short childrenCount;
};

extern struct ColliderCallbacks gCompoundColliderCallbacks;

void compoundColliderCollideObject(struct CollisionObject* compoundColliderObject, struct CollisionObject* other, struct ContactSolver* contactSolver);

#endif
