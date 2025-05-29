#ifndef __COLLISION_SCENE_H__
#define __COLLISION_SCENE_H__

#include "collision_object.h"
#include "defs.h"
#include "math/ray.h"
#include "point_constraint.h"
#include "raycasting.h"
#include "world.h"

#define PORTAL_COVER_HEIGHT_RADIUS 0.708084f
#define PORTAL_COVER_WIDTH_RADIUS  0.420425f

#define PORTAL_THICKNESS        0.11f

struct CollisionScene {
    struct CollisionObject* quads;
    struct World* world;
    short portalRooms[2];
    short portalColliderIndex[2];
    struct Transform* portalTransforms[2];
    struct Transform toOtherPortalTransform[2];
    struct Vector3 portalVelocity[2];
    struct CollisionObject* dynamicObjects[MAX_DYNAMIC_COLLISION];
    u16 dynamicObjectCount;
    u16 quadCount;
};

typedef void (*ManifoldCallback)(void* data, struct ContactManifold* contact);

extern struct CollisionScene gCollisionScene;

void collisionSceneInit(struct CollisionScene* scene, struct CollisionObject* quads, int quadCount, struct World* world);
void collisionObjectCollideMixed(struct CollisionObject* object, struct Vector3* objectPrevPos, struct Box3D* sweptBB, struct CollisionScene* scene, struct ContactSolver* contactSolver);
void collisionObjectCollidePairMixed(struct CollisionObject* a, struct Vector3* aPrevPos, struct Box3D* sweptA, struct CollisionObject* b, struct Vector3* bPrevPos, struct Box3D* sweptB, struct ContactSolver* contactSolver);

int collisionObjectCollideShapeCast(struct CollisionObject* object, struct Vector3* offset, struct CollisionScene* scene, struct Vector3* finalLocation);

int collisionSceneIsTouchingSinglePortal(struct Vector3* contactPoint, struct Vector3* contactNormal, struct Transform* portalTransform, int portalIndex);
int collisionSceneIsTouchingPortal(struct Vector3* contactPoint, struct Vector3* contactNormal);
int collisionSceneObjectIsTouchingPortal(struct CollisionObject* object, int portalIndex);
int collisionSceneIsPortalOpen();

void collisionSceneSetPortal(int portalIndex, struct Transform* transform, int roomIndex, int colliderIndex);
struct Transform* collisionSceneTransformToPortal(int fromPortal);

void collisionScenePushObjectsOutOfPortal(int portalIndex);

int collisionSceneRaycast(struct CollisionScene* scene, int roomIndex, struct Ray* ray, int collisionLayers, float maxDistance, int passThroughPortals, struct RaycastHit* hit);
int collisionSceneRaycastOnlyDynamic(struct CollisionScene* scene, struct Ray* ray, int collisionLayers, float maxDistance, struct RaycastHit* hit);

void collisionSceneGetPortalTransform(int fromPortal, struct Transform* out);

void collisionSceneAddDynamicObject(struct CollisionObject* object);
void collisionSceneRemoveDynamicObject(struct CollisionObject* object);
int collisionSceneDynamicObjectCount();

void collisionSceneUpdateDynamics();

#endif