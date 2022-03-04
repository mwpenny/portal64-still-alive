#ifndef __COLLISION_BOX_H__
#define __COLLISION_BOX_H__

#include "collision.h"
#include "collision_quad.h"
#include "../math/vector3.h"
#include "../math/transform.h"
#include "../math/plane.h"

struct CollisionBox {
    struct Vector3 sideLength;
};

void collisionBoxCollideQuad(struct CollisionBox* box, struct Transform* boxTransform, struct CollisionQuad* quad);

void collisionBoxCollidePlane(struct CollisionBox* box, struct Transform* boxTransform, struct Plane* plane, ContactCallback callback, void* callbackData);

#endif