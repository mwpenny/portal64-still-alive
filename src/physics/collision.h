#ifndef __COLLISION_H__
#define __COLLISION_H__

#include "../math/vector3.h"

enum CollisionShapeType {
    CollisionShapeTypeBox,
};

struct ContactPoint {
    struct Vector3 point;
    struct Vector3 normal;
    float intersectionDepth;
};

struct ColliderTypeData;

typedef float (*MomentOfInertiaCalculator)(struct ColliderTypeData* typeData, float mass);

struct ColliderTypeData {
    enum CollisionShapeType type;
    void* data;
    MomentOfInertiaCalculator mofICalculator;
};

typedef void (*ContactCallback)(void* data, struct ContactPoint* contact);

#endif