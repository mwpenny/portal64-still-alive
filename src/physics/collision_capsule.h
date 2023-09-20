#ifndef __COLLISION_CAPSULE_H__
#define __COLLISION_CAPSULE_H__

#include "collision.h"

extern struct ColliderCallbacks gCollisionCapsuleCallbacks;

struct CollisionCapsule {
    float radius;
    float extendDownward;
};

float collisionCapsuleSolidMofI(struct ColliderTypeData* typeData, float mass);

#endif