#ifndef __COLLISION_BOX_H__
#define __COLLISION_BOX_H__

#include "collision.h"
#include "collision_quad.h"
#include "contact_solver.h"
#include "math/plane.h"
#include "math/transform.h"
#include "math/vector3.h"

struct CollisionBox {
    struct Vector3 sideLength;
};

extern struct ColliderCallbacks gCollisionBoxCallbacks;

float collisionBoxSolidMofI(struct ColliderTypeData* typeData, float mass);

#endif