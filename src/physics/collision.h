#ifndef __COLLISION_H__
#define __COLLISION_H__

#include "../math/vector3.h"

struct ContactPoint {
    struct Vector3 point;
    struct Vector3 normal;
    float intersectionDepth;
};

typedef void (*ContactCallback)(void* data, struct ContactPoint* contact);

#endif