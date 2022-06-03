#ifndef __COLLISION_OBJECT_H__
#define __COLLISION_OBJECT_H__

#include "rigid_body.h"
#include "collision.h"
#include "../math/box3d.h"

struct CollisionObject {
    struct ColliderTypeData *collider;
    struct RigidBody* body;
    struct Box3D boundingBox;
};

void collisionObjectInit(struct CollisionObject* object, struct ColliderTypeData *collider, struct RigidBody* body, float mass);

void collisionObjectCollideWithQuad(struct CollisionObject* object, struct CollisionObject* quad, struct ContactSolver* contactSolver);
void collisionObjectCollideTwoObjects(struct CollisionObject* a, struct CollisionObject* b, struct ContactSolver* contactSolver);

void collisionObjectUpdateBB(struct CollisionObject* object);

// data should be of type struct CollisionQuad
int minkowsiSumAgainstQuad(void* data, struct Vector3* direction, struct Vector3* output);

// data should be of type struct CollisionObject
int minkowsiSumAgainstObject(void* data, struct Vector3* direction, struct Vector3* output);


#endif