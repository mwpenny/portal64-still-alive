#ifndef __COLLISION_SCENE_H__
#define __COLLISION_SCENE_H__

#include "collision_object.h"
#include "raycasting.h"
#include "../math/ray.h"

#define PORTAL_THICKNESS        0.11f
#define PORTAL_X_RADIUS         0.5f

#define MAX_DYNAMIC_OBJECTS     16

struct CollisionScene {
    struct CollisionObject* quads;
    struct Transform* portalTransforms[2];
    struct CollisionObject* dynamicObjects[MAX_DYNAMIC_OBJECTS];
    u16 dynamicObjectCount;
    u16 quadCount;
};

typedef void (*ManifoldCallback)(void* data, struct ContactConstraintState* contact);

extern struct CollisionScene gCollisionScene;

void collisionSceneInit(struct CollisionScene* scene, struct CollisionObject* quads, int quadCount);
void collisionObjectCollideWithScene(struct CollisionObject* object, struct CollisionScene* scene, struct ContactSolver* contactSolver);

int collisionSceneIsTouchingPortal(struct Vector3* contactPoint);
int collisionSceneIsPortalOpen();

void collisionObjectQueryScene(struct CollisionObject* object, struct CollisionScene* scene, void* data, ManifoldCallback callback);

int collisionSceneRaycast(struct CollisionScene* scene, struct Ray* ray, float maxDistance, int passThroughPortals, struct RaycastHit* hit);

void collisionSceneGetPortalTransform(int fromPortal, struct Transform* out);

void collisionSceneAddDynamicObject(struct CollisionObject* object);
void collisionSceneRemoveDynamicObject(struct CollisionObject* object);

void collisionSceneUpdateDynamics();

#endif