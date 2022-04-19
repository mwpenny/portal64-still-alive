#ifndef __COLLISION_SCENE_H__
#define __COLLISION_SCENE_H__

#include "collision_object.h"

struct CollisionScene {
    struct CollisionObject* quads;
    int quadCount;
};

typedef void (*ManifoldCallback)(void* data, struct ContactConstraintState* contact);

extern struct CollisionScene gCollisionScene;

void collisionSceneInit(struct CollisionScene* scene, struct CollisionObject* quads, int quadCount);
void collisionObjectCollideWithScene(struct CollisionObject* object, struct CollisionScene* scene, struct ContactSolver* contactSolver);

void collisionObjectQueryScene(struct CollisionObject* object, struct CollisionScene* scene, void* data, ManifoldCallback callback);

#endif