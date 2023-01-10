#ifndef __COLLISION_SCENE_H__
#define __COLLISION_SCENE_H__

#include "collision_object.h"
#include "raycasting.h"
#include "../math/ray.h"
#include "world.h"
#include "defs.h"
#include "point_constraint.h"

#define PORTAL_THICKNESS        0.11f
#define PORTAL_X_RADIUS         0.5f

struct CollisionScene {
    struct CollisionObject* quads;
    struct World* world;
    short portalRooms[2];
    struct Transform* portalTransforms[2];
    struct Vector3 portalVelocity[2];
    struct CollisionObject* dynamicObjects[MAX_DYNAMIC_OBJECTS];
    u16 dynamicObjectCount;
    u16 quadCount;
};

typedef void (*ManifoldCallback)(void* data, struct ContactManifold* contact);

extern struct CollisionScene gCollisionScene;

void collisionSceneInit(struct CollisionScene* scene, struct CollisionObject* quads, int quadCount, struct World* world);
void collisionObjectCollideWithScene(struct CollisionObject* object, struct CollisionScene* scene, struct ContactSolver* contactSolver);
void collisionObjectCollideWithSceneSwept(struct CollisionObject* object, struct Vector3* objectPrevPos, struct Box3D* sweptBB, struct CollisionScene* scene, struct ContactSolver* contactSolver);
void collisionObjectCollideMixed(struct CollisionObject* object, struct Vector3* objectPrevPos, struct Box3D* sweptBB, struct CollisionScene* scene, struct ContactSolver* contactSolver);

int collisionSceneIsTouchingSinglePortal(struct Vector3* contactPoint, struct Vector3* contactNormal, struct Transform* portalTransform, int portalIndex);
int collisionSceneIsTouchingPortal(struct Vector3* contactPoint, struct Vector3* contactNormal);
int collisionSceneIsPortalOpen();

int collisionSceneRaycast(struct CollisionScene* scene, int roomIndex, struct Ray* ray, int collisionLayers, float maxDistance, int passThroughPortals, struct RaycastHit* hit);

void collisionSceneGetPortalTransform(int fromPortal, struct Transform* out);

void collisionSceneAddDynamicObject(struct CollisionObject* object);
void collisionSceneRemoveDynamicObject(struct CollisionObject* object);

void collisionSceneUpdateDynamics();

#endif