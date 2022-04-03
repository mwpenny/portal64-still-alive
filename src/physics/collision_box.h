#ifndef __COLLISION_BOX_H__
#define __COLLISION_BOX_H__

#include "collision.h"
#include "collision_quad.h"
#include "../math/vector3.h"
#include "../math/transform.h"
#include "../math/plane.h"
#include "contact_solver.h"

struct CollisionBox {
    struct Vector3 sideLength;
};

extern struct ColliderCallbacks gCollisionBoxCallbacks;

int collisionBoxCollidePlane(void* data, struct Transform* boxTransform, struct Plane* plane, struct ContactConstraintState* contact);

float collisionBoxSolidMofI(struct ColliderTypeData* typeData, float mass);

#endif