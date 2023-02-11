#ifndef __DECOR_OBJECT_H__
#define __DECOR_OBJECT_H__

#include <ultra64.h>
#include "../physics/collision_object.h"
#include "../math/transform.h"

enum DecorObjectFlags {
    // important objects respawn at their original 
    // location if they escape the level
    DecorObjectFlagsImportant = (1 << 0),
};

struct DecorObjectDefinition {
    struct ColliderTypeData colliderType;
    float mass;
    float radius;
    Gfx* graphics;
    short materialIndex;
    short materialIndexFizzled;
    short soundClipId;
    short flags;
};

struct DecorObject {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    struct DecorObjectDefinition* definition;
    struct Vector3 originalPosition;
    struct Quaternion originalRotation;
    short originalRoom;
    short dynamicId;
    ALSndId playingSound;
    float fizzleTime;
};

struct DecorObject* decorObjectNew(struct DecorObjectDefinition* definition, struct Transform* at, int room);
void decorObjectInit(struct DecorObject* object, struct DecorObjectDefinition* definition, struct Transform* at, int room);
void decorObjectClenaup(struct DecorObject* decorObject);
void decorObjectDelete(struct DecorObject* decorObject);

int decorObjectUpdate(struct DecorObject* decorObject);

#endif