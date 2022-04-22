#ifndef __COLLISION_H__
#define __COLLISION_H__

#include "../math/vector3.h"
#include "../math/transform.h"
#include "../math/plane.h"
#include "../math/ray.h"
#include "contact_solver.h"
#include "collision_quad.h"

enum CollisionShapeType {
    CollisionShapeTypeBox,
    CollisionShapeTypeQuad,
    CollisionShapeTypeSphere,
};

struct ContactPoint {
    struct Vector3 point;
    struct Vector3 normal;
    float intersectionDepth;
};

struct ColliderTypeData;
struct CollisionObject;
struct RaycastHit;

typedef float (*MomentOfInertiaCalculator)(struct ColliderTypeData* typeData, float mass);


typedef int (*CollideWithPlane)(void* data, struct Transform* transform, struct Plane* plane, struct ContactConstraintState* contact);
typedef int (*CollideWithQuad)(void* data, struct Transform* transform, struct CollisionQuad* quad, struct ContactConstraintState* contact);

typedef int (*RaycastCollider)(struct CollisionObject* object, struct Ray* ray, float maxDistance, struct RaycastHit* contact);

struct ColliderCallbacks {
    CollideWithPlane collideWithPlane;
    CollideWithQuad collideWithQuad;
    RaycastCollider raycast;
    MomentOfInertiaCalculator mofICalculator;
};

struct ColliderTypeData {
    enum CollisionShapeType type;
    void* data;
    float bounce;
    float friction;
    struct ColliderCallbacks* callbacks;
};

typedef void (*ContactCallback)(void* data, struct ContactPoint* contact);

#endif