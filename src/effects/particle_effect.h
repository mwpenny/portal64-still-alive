#ifndef __PARTICLE_EFFECT_H__
#define __PARTICLE_EFFECT_H__

#include "graphics/color.h"
#include "math/vector3.h"

#define MAX_PARTICLES   16

enum ParticleFlags {
    ParticleFlagsBillboarded        = (1 << 0),
    ParticleFlagsNoGravity          = (1 << 1),
};

enum ParticleCullingType {
    // Check frustum and starting room visibility.
    // Good if effect doesn't cross doorways.
    ParticleCullingTypeStandard,

    // Stop if room is hidden on first viewing. Visible in all rooms afterward.
    // Tradeoff to prevent pop in/out at doorways + not start in multiple rooms.
    // Good for effects that cross doorways.
    ParticleCullingTypeCrossRooms,
};

enum ParticleCullingFlags {
    ParticleCullingFlagsCrossRooms  = (1 << 0),
    ParticleCullingFlagsWasSeen     = (1 << 1),
    ParticleCullingFlagsRoomWasSeen = (1 << 2),
};

struct ParticleEffectDefinition {
    float lifetime;
    float fadeInEndTime;
    float fadeOutStartTime;
    float fullWidthTime;
    float tailDelay;
    float minNormalVelocity;
    float maxNormalVelocity;
    float minTangentVelocity;
    float maxTangentVelocity;
    short count;
    short materialIndex;
    float halfWidth;
    struct Coloru8 color;
    enum ParticleFlags flags;
};

struct Particle {
    struct Vector3 position[2];
    struct Vector3 velocity;
    struct Vector3 widthOffset;
};

struct ParticleEffect {
    struct ParticleEffectDefinition* definition;
    struct Particle particles[MAX_PARTICLES];
    struct Vector3 startPosition;
    struct Vector3* position;
    struct Transform* parent;
    float time;
    short roomIndex;
    short dynamicId;
    enum ParticleCullingFlags cullingFlags;
};

void particleEffectInit(struct ParticleEffect* effect);
void particleEffectPlay(
    struct ParticleEffect* effect,
    struct ParticleEffectDefinition* definition,
    struct Vector3* origin,
    struct Vector3* normal,
    struct Transform* parent,
    int roomIndex,
    enum ParticleCullingType cullingType
);
void particleEffectUpdate(struct ParticleEffect* effect);

#endif
