#ifndef __COLLISION_CAPSULE_H__
#define __COLLISION_CAPSULE_H__

#include "collision.h"

struct CollisionCapsule {
    float radius;
    float extendDownward;
};

extern struct ColliderCallbacks gCollisionCapsuleCallbacks;

#endif
