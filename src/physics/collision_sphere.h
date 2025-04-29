#ifndef __COLLISION_SPHERE_H__
#define __COLLISION_SPHERE_H__

#include "collision.h"

struct CollisionSphere {
    float radius;
};

extern struct ColliderCallbacks gCollisionSphereCallbacks;

#endif