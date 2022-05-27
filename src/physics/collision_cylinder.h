#ifndef __COLLISION_CYLINDER_H__
#define __COLLISION_CYLINDER_H__

#include "collision_quad.h"
#include "collision.h"

struct CollisionCylinder {
    float radius;
    float halfHeight;
};

extern struct ColliderCallbacks gCollisionCylinderCallbacks;

int collisionCylinderCollideQuad(void* data, struct Transform* cylinderTransform, struct CollisionQuad* quad, struct ContactConstraintState* output);
float collisionCylinderSolidMofI(struct ColliderTypeData* typeData, float mass);
void collisionCylinderBoundingBox(struct ColliderTypeData* typeData, struct Transform* transform, struct Box3D* box);

#endif