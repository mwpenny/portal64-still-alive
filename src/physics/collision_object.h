#ifndef __COLLISION_OBJECT_H__
#define __COLLISION_OBJECT_H__

#include "collision.h"
#include "math/box3d.h"
#include "rigid_body.h"

#define COLLISION_LAYERS_STATIC             (1 << 0)
#define COLLISION_LAYERS_TRANSPARENT        (1 << 1)
#define COLLISION_LAYERS_TANGIBLE           (1 << 2)
#define COLLISION_LAYERS_GRABBABLE          (1 << 3)
#define COLLISION_LAYERS_FIZZLER            (1 << 4)
#define COLLISION_LAYERS_BLOCK_PORTAL       (1 << 5)
#define COLLISION_LAYERS_BLOCK_BALL         (1 << 6)
#define COLLISION_LAYERS_BLOCK_TURRET_SIGHT (1 << 7)
#define COLLISION_LAYERS_BLOCK_TURRET_SHOTS (1 << 8)

#define COLLISION_OBJECT_HAS_CONTACTS       (1 << 0)
#define COLLISION_OBJECT_PLAYER_STANDING    (1 << 1)
#define COLLISION_OBJECT_INTERACTED         (1 << 2)

typedef void (*TriggerCallback)(struct CollisionObject* collisionObject, struct CollisionObject* objectEnteringTrigger);
typedef void (*SweptCollideCallback)(struct CollisionObject* collisionObject, float velocityMag);

struct CollisionObject {
    struct ColliderTypeData *collider;
    struct RigidBody* body;
    struct Vector3* position;
    struct Box3D boundingBox;
    short collisionLayers;
    short flags;
    void* data;
    TriggerCallback trigger;
    SweptCollideCallback sweptCollide;
    u32 manifoldIds;
};

struct SweptCollisionObject {
    struct CollisionObject* object;
    struct Vector3* prevPos;
};

int collisionObjectIsActive(struct CollisionObject* object);
int collisionObjectIsGrabbable(struct CollisionObject* object);
int collisionObjectShouldGenerateContacts(struct CollisionObject* object);

void collisionObjectInit(struct CollisionObject* object, struct ColliderTypeData *collider, struct RigidBody* body, float mass, int collisionLayers);
void collisionObjectReInit(struct CollisionObject* object, struct ColliderTypeData *collider, struct RigidBody* body, float mass, int collisionLayers);

struct ContactManifold* collisionObjectCollideWithQuad(struct CollisionObject* object, struct CollisionObject* quad, struct ContactSolver* contactSolver, int shouldCheckPortals);
struct ContactManifold* collisionObjectCollideWithQuadSwept(struct CollisionObject* object, struct Vector3* objectPrevPos, struct Box3D* sweptBB, struct CollisionObject* quadObject, struct ContactSolver* contactSolver, int shouldCheckPortals);
void collisionObjectCollideTwoObjects(struct CollisionObject* a, struct CollisionObject* b, struct ContactSolver* contactSolver);
void collisionObjectCollideTwoObjectsSwept(
    struct CollisionObject* a, 
    struct Vector3* prevAPos, 
    struct Box3D* sweptA,
    struct CollisionObject* b, 
    struct Vector3* prevBPos, 
    struct Box3D* sweptB,
    struct ContactSolver* contactSolver
);

enum SweptCollideResult {
    SweptCollideResultMiss,
    SweptCollideResultOverlap,
    SweptCollideResultHit,
};

struct EpaResult;

enum SweptCollideResult collisionObjectSweptCollide(
    struct CollisionObject* object, 
    struct Vector3* objectPrevPos, 
    struct Box3D* sweptBB, 
    struct CollisionObject* quadObject, 
    int shouldCheckPortals, 
    struct EpaResult* result, 
    struct Vector3* objectEnd
);

void collisionObjectUpdateBB(struct CollisionObject* object);

// data should be of type struct CollisionQuad
int quadMinkowskiSupport(void* data, struct Vector3* direction, struct Vector3* output);

// data should be of type struct CollisionObject
int objectMinkowskiSupport(void* data, struct Vector3* direction, struct Vector3* output);

// data should be of type struct SweptCollisionObject
int sweptObjectMinkowskiSupport(void* data, struct Vector3* direction, struct Vector3* output);

void collisionObjectLocalRay(struct CollisionObject* object, struct Ray* ray, struct Ray* localRay);

// Add object's body offset (if any) to output vector
void collisionObjectAddBodyOffset(struct CollisionObject* object, struct Vector3* out);

#endif