#ifndef __DECOR_OBJECT_H__
#define __DECOR_OBJECT_H__

#include <ultra64.h>

#include "audio/soundplayer.h"
#include "graphics/renderstate.h"
#include "math/transform.h"
#include "physics/collision_object.h"

enum DecorObjectFlags {
    // Important objects respawn at their original location if they escape the level
    DecorObjectFlagsImportant = (1 << 0),
    DecorObjectFlagsMuted = (1 << 1),
};

enum FizzleCheckResult {
    FizzleCheckResultNone,
    FizzleCheckResultStart,
    FizzleCheckResultEnd,
};

struct DecorObjectDefinition {
    struct ColliderTypeData colliderType;
    float mass;
    float radius;
    short dynamicModelIndex;
    short materialIndex;
    short materialIndexFizzled;
    short soundClipId;
    short soundFizzleId;
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
    float fizzleTime;
    SoundId playingSound;
};

void decorObjectInit(struct DecorObject* object, struct DecorObjectDefinition* definition, struct Transform* at, int room);
int decorObjectUpdate(struct DecorObject* decorObject);
void decorObjectOnDeserialize(struct DecorObject* decorObject);

struct DecorObject* decorObjectNew(struct DecorObjectDefinition* definition, struct Transform* at, int room);
void decorObjectCleanup(struct DecorObject* decorObject);
void decorObjectDelete(struct DecorObject* decorObject);

Gfx* decorBuildFizzleGfx(Gfx* gfxToRender, float fizzleTime, struct RenderState* renderState);
enum FizzleCheckResult decorObjectUpdateFizzler(struct CollisionObject* collisionObject, float* fizzleTime);

#endif
