#ifndef __DECOR_OBJECT_H__
#define __DECOR_OBJECT_H__

#include <ultra64.h>
#include "../physics/collision_object.h"
#include "../math/transform.h"

struct DecorObjectDefinition {
    struct ColliderTypeData colliderType;
    float mass;
    float radius;
    Gfx* graphics;
    short materialIndex;
};

struct DecorObject {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    struct DecorObjectDefinition* definition;
    short dynamicId;
};

struct DecorObject* decorObjectNew(struct DecorObjectDefinition* definition, struct Transform* at, int room);
void decorObjectInit(struct DecorObject* object, struct DecorObjectDefinition* definition, struct Transform* at, int room);
void decorObjectDelete(struct DecorObject* decorObject);

#endif