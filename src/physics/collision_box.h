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

float collisionBoxSolidMofI(struct ColliderTypeData* typeData, float mass);

void collisionBoxBoundingBox(struct ColliderTypeData* typeData, struct Transform* transform, struct Box3D* box);

int collisionBoxMinkowsiSum(void* data, struct Basis* basis, struct Vector3* direction, struct Vector3* output);

#endif