#ifndef __COLLISION_SPHERE_H__
#define __COLLISION_SPHERE_H__

#include "collision.h"

extern struct ColliderCallbacks gCollisionSphereCallbacks;

struct CollisionSphere {
    float radius;
};

int collisionSphereCollideQuad(void* data, struct Transform* boxTransform, struct CollisionQuad* quad, struct ContactManifold* output);
float collisionSphereSolidMofI(struct ColliderTypeData* typeData, float mass);
int collisionSphereCheckWithNearestPoint(struct Vector3* nearestPoint, struct CollisionSphere* otherSphere, struct Vector3* spherePos, struct ContactManifold* contact);
int collisionSphereCollideWithSphere(void* data, struct Transform* transform, struct CollisionSphere* otherSphere, struct Vector3* spherePos, struct ContactManifold* contact);

#endif