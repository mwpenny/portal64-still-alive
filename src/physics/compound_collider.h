#ifndef __PHYSICS_COMPOUND_COLLIDER_H__
#define __PHYSICS_COMPOUND_COLLIDER_H__

#include "collision_object.h"

#define COMPOUND_COLLIDER_MAX_CHILD_COUNT 4

struct CollisionScene;

struct CompoundColliderComponentDefinition {
    struct ColliderTypeData collider;
    struct Vector3 parentOffset;
};

struct CompoundColliderDefinition {
    struct CompoundColliderComponentDefinition* childDefinitions;
    short childrenCount;
};

struct CompoundColliderComponent {
    struct CollisionObject object;
    struct Vector3* parentOffset;
    struct Vector3 position;
};

struct CompoundCollider {
    struct CompoundColliderComponent children[COMPOUND_COLLIDER_MAX_CHILD_COUNT];
    short childrenCount;

    struct ColliderTypeData colliderType;
};

extern struct ColliderCallbacks gCompoundColliderCallbacks;

void compoundColliderInit(
    struct CompoundCollider* collider,
    struct CompoundColliderDefinition* definition,
    struct RigidBody* body,
    int collisionLayers
);

void compoundColliderFurthestPoint(
    struct CollisionObject* compoundColliderObject,
    struct Vector3* direction,
    struct Vector3* point
);

int compoundColliderHasOverlap(
    struct CollisionObject* compoundColliderObject,
    void* other,
    MinkowskiSupport otherSupport,
    struct Vector3* firstDirection
);

void compoundColliderCollideMixed(
    struct CollisionObject* compoundColliderObject,
    struct Vector3* prevCompoundColliderPos,
    struct Box3D* sweptCompoundCollider,
    struct CollisionScene* scene,
    struct ContactSolver* contactSolver
);

void compoundColliderCollidePairMixed(
    struct CollisionObject* compoundColliderObject,
    struct Vector3* prevCompoundColliderPos,
    struct Box3D* sweptCompoundCollider,
    struct CollisionObject* other,
    struct Vector3* prevOtherPos,
    struct Box3D* sweptOther,
    struct ContactSolver* contactSolver
);

#endif
