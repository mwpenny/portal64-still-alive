#ifndef __COLLISION_CYLINDER_H__
#define __COLLISION_CYLINDER_H__

#include "collision_quad.h"
#include "collision.h"
#include "../math/vector2.h"

struct CollisionCylinder {
    float radius;
    float halfHeight;
    struct Vector2* edgeVectors;
    int edgeCount;
    struct CollisionQuad* outsideFaces;
};

extern struct ColliderCallbacks gCollisionCylinderCallbacks;

float collisionCylinderSolidMofI(struct ColliderTypeData* typeData, float mass);
void collisionCylinderBoundingBox(struct ColliderTypeData* typeData, struct Transform* transform, struct Box3D* box);
int collisionCylinderMinkowsiSum(void* data, struct Basis* basis, struct Vector3* direction, struct Vector3* output);
int collisionCylinderRaycast(struct CollisionObject* cylinderObject, struct Ray* ray, float maxDistance, struct RaycastHit* contact);

#endif