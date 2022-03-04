#ifndef __RIGID_BODY_H__
#define __RIGID_BODY_H__

#include "../math/transform.h"

struct RigidBody {
    struct Transform transform;
    struct Vector3 velocity;
    struct Vector3 angularVelocity;

    float mass;
};

#endif