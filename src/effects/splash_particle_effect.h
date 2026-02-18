#ifndef __SPLASH_PARTICLE_EFFECT_H__
#define __SPLASH_PARTICLE_EFFECT_H__

#include "graphics/color.h"
#include "math/vector3.h"

enum SplashParticleFlags {
    SplashParticleFlagsBillboarded = (1 << 0),
    SplashParticleFlagsNoGravity   = (1 << 1)
};

struct SplashParticleDefinition {
    float particleLifetime;
    float fadeStartTime;
    float fullWidthTime;
    float particleTailDelay;
    float minNormalVelocity;
    float maxNormalVelocity;
    float minTangentVelocity;
    float maxTangentVelocity;
    short particleCount;
    short materialIndex;
    float particleHalfWidth;
    struct Coloru8 particleColor;
    enum SplashParticleFlags flags;
};

struct SplashParticle {
    struct Vector3 position[2];
    struct Vector3 velocity;
    struct Vector3 widthOffset;
};

#define MAX_SPLASH_PARTICLES    16

struct SplashParticleEffect {
    struct SplashParticleDefinition* def;
    struct SplashParticle particles[MAX_SPLASH_PARTICLES];
    struct Vector3 startPosition;
    struct Vector3* position;
    struct Transform* parent;
    float time;
    short dynamicId;
};

void splashParticleEffectInit(struct SplashParticleEffect* effect);
void splashParticleEffectPlay(struct SplashParticleEffect* effect, struct SplashParticleDefinition* definition, struct Vector3* origin, struct Vector3* normal, struct Transform* parent);
void splashParticleEffectUpdate(struct SplashParticleEffect* effect);

#endif