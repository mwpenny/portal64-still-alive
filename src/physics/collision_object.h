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

void collisionObjectCollideWithPlane(struct CollisionObject* object, struct CollisionObject* plane, struct ContactSolver* contactSolver);
void collisionObjectCollideWithQuad(struct CollisionObject* object, struct CollisionObject* quad, struct ContactSolver* contactSolver);

void collisionObjectUpdateBB(struct CollisionObject* object);

struct MinkowsiSumAgainstQuad {
    struct CollisionObject* collisionObject;
    struct CollisionQuad* quad;
};

void minkowsiSumAgainstQuadInit(struct MinkowsiSumAgainstQuad* sumData, struct CollisionObject* object, struct CollisionQuad* quad);

// data should be of type struct MinkowsiSumAgainstQuad
void minkowsiSumAgainstQuadSum(void* data, struct Vector3* direction, struct Vector3* output);

struct MinkowsiSumAgainstObjects {
    struct CollisionObject* collisionObjectA;
    struct CollisionObject* collisionObjectB;
};

void minkowsiSumAgainstObjectsInit(struct MinkowsiSumAgainstObjects* sumData, struct CollisionObject* a, struct CollisionObject* b);

// data should be of type struct MinkowsiSumAgainstObjects
void minkowsiSumAgainstObjects(void* data, struct Vector3* direction, struct Vector3* output);


#endif