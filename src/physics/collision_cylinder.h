#ifndef __COLLISION_CYLINDER_H__
#define __COLLISION_CYLINDER_H__

#include "collision_quad.h"

struct CollisionCylinder {
    float radius;
    float halfHeight;
};


int collisionCylinderCollideQuad(void* data, struct Transform* cylinderTransform, struct CollisionQuad* quad, struct ContactConstraintState* output);

#endif