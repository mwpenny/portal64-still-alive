#ifndef __COLLISION_H__
#define __COLLISION_H__

#include "../math/basis.h"
#include "../math/box3d.h"
#include "../math/plane.h"
#include "../math/ray.h"
#include "../math/transform.h"
#include "../math/vector3.h"
#include "contact_solver.h"
#include "collision_quad.h"
#include "gjk.h"

enum CollisionShapeType {
    CollisionShapeTypeBox,
    CollisionShapeTypeQuad,
    CollisionShapeTypeSphere,
    CollisionShapeTypeCylinder,
    CollisionShapeTypeMesh,
};

struct ColliderTypeData;
struct CollisionObject;
struct RaycastHit;
struct CollisionSphere;

typedef float (*MomentOfInertiaCalculator)(struct ColliderTypeData* typeData, float mass);
typedef void (*BoundingBoxCalculator)(struct ColliderTypeData* typeData, struct Transform* transform, struct Box3D* box);

typedef int (*CollideWithQuad)(void* data, struct Transform* transform, struct CollisionQuad* quad, struct ContactManifold* contact);

typedef int (*RaycastCollider)(struct CollisionObject* object, struct Ray* ray, float maxDistance, struct RaycastHit* contact);

typedef int (*MinkowsiSumWithBasis)(void* data, struct Basis* basis, struct Vector3* direction, struct Vector3* output);

typedef int (*CollideWithSphere)(void* data, struct Transform* transform, struct CollisionSphere* sphere, struct Vector3* spherePos, struct ContactManifold* contact);

struct ColliderCallbacks {
    RaycastCollider raycast;
    MomentOfInertiaCalculator mofICalculator;
    BoundingBoxCalculator boundingBoxCalculator;
    MinkowsiSumWithBasis minkowsiSum;
    CollideWithSphere collideWithSphere;
};

struct ColliderTypeData {
    enum CollisionShapeType type;
    void* data;
    float bounce;
    float friction;
    struct ColliderCallbacks* callbacks;
};

#endif