#ifndef __PHYSICS_MESH_COLLIDER_H__
#define __PHYSICS_MESH_COLLIDER_H__

#include "./collision_object.h"
#include "./contact_solver.h"
#include "../math/box3d.h"

struct MeshCollider {
    struct CollisionObject* children;
    int childrenCount;
    struct Vector3 localCenter;
    struct Vector3 localHalfBoundingbox;
    float radiusFromCenter;
};

extern struct ColliderCallbacks gMeshColliderCallbacks;

void meshColliderCollideObject(struct CollisionObject* meshColliderObject, struct CollisionObject* other, struct ContactSolver* contactSolver);
int meshColliderRaycast(struct CollisionObject* object, struct Ray* ray, float maxDistance, struct RaycastHit* contact);

#endif