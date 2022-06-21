#ifndef __FIZZLER_H__
#define __FIZZLER_H__

#include <ultra64.h>
#include "../math/transform.h"
#include "defs.h"
#include "../util/time.h"

#include "../physics/collision_box.h"
#include "../physics/collision_object.h"

#define FIZZLER_PARTICLES_PER_1x1   2.5f
#define FIZZLER_PARTICLE_VELOCITY   1.0f
#define FIZZLER_UNITS_PER_UPDATE    (int)(SCENE_SCALE * FIZZLER_PARTICLE_VELOCITY * FIXED_DELTA_TIME)
#define FIZZLER_PARTICLE_LENGTH     0.4f
#define FIZZLER_PARTICLE_LENGTH_FIXED   (int)(FIZZLER_PARTICLE_LENGTH * SCENE_SCALE)
#define FIZZLER_PARTICLE_HEIGHT_FIXED   (int)(FIZZLER_PARTICLE_LENGTH * SCENE_SCALE * 0.5f)

#define IMAGE_WIDTH     16
#define IMAGE_HEIGHT    32

struct Fizzler {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    struct ColliderTypeData colliderType;
    struct CollisionBox collisionBox;
    Vtx* modelVertices;
    Gfx* modelGraphics;
    short particleCount;
    short maxExtent;
    short maxVerticalExtent;
    short oldestParticleIndex;
    short dynamicId;
};

void fizzlerInit(struct Fizzler* fizzler, struct Transform* transform, float width, float height, int room);
void fizzlerUpdate(struct Fizzler* fizzler);

#endif