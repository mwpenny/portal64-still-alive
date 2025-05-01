#ifndef __COLLISION_H__
#define __COLLISION_H__

#include "collision_quad.h"
#include "contact_solver.h"
#include "gjk.h"
#include "math/basis.h"
#include "math/box3d.h"
#include "math/plane.h"
#include "math/ray.h"
#include "math/transform.h"
#include "math/vector3.h"

enum CollisionShapeType {
    CollisionShapeTypeNone,
    CollisionShapeTypeBox,
    CollisionShapeTypeQuad,
    CollisionShapeTypeSphere,
    CollisionShapeTypeCapsule,
    CollisionShapeTypeCylinder,
    CollisionShapeTypeCompound,
    CollisionShapeTypeMesh,
};

struct ColliderTypeData;
struct RaycastHit;

typedef int   (*RaycastCollider)(struct CollisionObject* object, struct Ray* ray, float maxDistance, struct RaycastHit* contact);
typedef float (*MomentOfInertiaCalculator)(struct ColliderTypeData* typeData, float mass);
typedef void  (*BoundingBoxCalculator)(struct ColliderTypeData* typeData, struct Transform* transform, struct Box3D* box);
typedef int   (*MinkowskiSupportWithBasis)(void* data, struct Basis* basis, struct Vector3* direction, struct Vector3* output);

struct ColliderCallbacks {
    RaycastCollider raycast;
    MomentOfInertiaCalculator mofICalculator;
    BoundingBoxCalculator boundingBoxCalculator;
    MinkowskiSupportWithBasis minkowskiSupport;
};

struct ColliderTypeData {
    enum CollisionShapeType type;
    void* data;
    float bounce;
    float friction;
    struct ColliderCallbacks* callbacks;
};

#endif