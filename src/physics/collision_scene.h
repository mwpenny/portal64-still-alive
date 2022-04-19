#ifndef __COLLISION_SCENE_H__
#define __COLLISION_SCENE_H__

#include "collision_object.h"

struct CollisionScene {
    struct CollisionObject* quads;
    int quadCount;
};

extern struct CollisionScene gCollisionScene;

void collisionSceneInit(struct CollisionScene* scene, struct CollisionObject* quads, int quadCount);
void collisionObjectCollideWithScene(struct CollisionObject* object, struct CollisionScene* scene, struct ContactSolver* contactSolver);

#endif