#ifndef __COLLISION_SCENE_H__
#define __COLLISION_SCENE_H__

#include "collision_object.h"
#include "raycasting.h"

#define PORTAL_THICKNESS        0.11f
#define PORTAL_X_RADIUS         0.5f

struct CollisionScene {
    struct CollisionObject* quads;
    int quadCount;
    struct Transform* portalTransforms[0];
};

typedef void (*ManifoldCallback)(void* data, struct ContactConstraintState* contact);

extern struct CollisionScene gCollisionScene;

void collisionSceneInit(struct CollisionScene* scene, struct CollisionObject* quads, int quadCount);
void collisionObjectCollideWithScene(struct CollisionObject* object, struct CollisionScene* scene, struct ContactSolver* contactSolver);

int collisionSceneIsTouchingPortal(struct Vector3* contactPoint);
int collisionSceneIsPortalOpen();

void collisionObjectQueryScene(struct CollisionObject* object, struct CollisionScene* scene, void* data, ManifoldCallback callback);

int collisionSceneRaycast(struct CollisionScene* scene, struct Vector3* at, struct Vector3* dir, float maxDistance, int passThroughPortals, struct RaycastHit* hit);

void collisionSceneGetPortalTransform(int fromPortal, struct Transform* out);

#endif