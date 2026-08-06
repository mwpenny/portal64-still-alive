#include "effect_definitions.h"

#include "codegen/assets/materials/static.h"

struct ParticleEffectDefinition gFailPortalSplash[2] = {
    {
        .lifetime = 0.5f,
        .fullWidthTime = 0.125f,
        .fadeInEndTime = 0.0f,
        .fadeOutStartTime = 0.25f,
        .tailDelay = 0.1f,
        .minNormalVelocity = 0.5f,
        .maxNormalVelocity = 1.0f,
        .minTangentVelocity = 1.0f,
        .maxTangentVelocity = 2.0f,
        .count = 16,
        .materialIndex = PORTAL_1_PARTICLE_INDEX,
        .halfWidth = 0.05f,
        .color = {200, 100, 50, 255},
        .flags = 0,
    },
    {
        .lifetime = 0.5f,
        .fullWidthTime = 0.125f,
        .fadeInEndTime = 0.0f,
        .fadeOutStartTime = 0.25f,
        .tailDelay = 0.1f,
        .minNormalVelocity = 0.5f,
        .maxNormalVelocity = 1.0f,
        .minTangentVelocity = 1.0f,
        .maxTangentVelocity = 2.0f,
        .count = 16,
        .materialIndex = PORTAL_1_PARTICLE_INDEX,
        .halfWidth = 0.05f,
        .color = {50, 70, 200, 255},
        .flags = 0,
    },
};

struct ParticleEffectDefinition gBallBurst = {
    .lifetime = 2.0f,
    .fullWidthTime = 0.125f,
    .fadeInEndTime = 0.0f,
    .fadeOutStartTime = 1.5f,
    .tailDelay = 0.1f,
    .minNormalVelocity = -1.0f,
    .maxNormalVelocity = 6.0f,
    .minTangentVelocity = 0.5f,
    .maxTangentVelocity = 1.0f,
    .count = 16,
    .materialIndex = SPARK_INDEX,
    .halfWidth = 0.02f,
    .color = {255, 255, 255, 255},
    .flags = 0,
};


struct ParticleEffectDefinition gBallBounce = {
    .lifetime = 0.75f,
    .fullWidthTime = 0.125f,
    .fadeInEndTime = 0.0f,
    .fadeOutStartTime = 0.5f,
    .tailDelay = 0.1f,
    .minNormalVelocity = 0.5f,
    .maxNormalVelocity = 1.5f,
    .minTangentVelocity = 0.5f,
    .maxTangentVelocity = 1.0f,
    .count = 16,
    .materialIndex = SPARK_INDEX,
    .halfWidth = 0.02f,
    .color = {255, 255, 255, 255},
    .flags = 0,
};

struct ParticleEffectDefinition gMuzzleFlash = {
    .lifetime = 0.1f,
    .fullWidthTime = 0.075f,
    .fadeInEndTime = 0.0f,
    .fadeOutStartTime = 0.025f,
    .tailDelay = 0.8f,
    .minNormalVelocity = 0.45f,
    .maxNormalVelocity = 0.7f,
    .minTangentVelocity = 0.06f,
    .maxTangentVelocity = 0.0725f,
    .count = 4,
    .materialIndex = MUZZLEFLASH2_INDEX,
    .halfWidth = 0.15f,
    .color = {210, 135, 50, 255},
    .flags = ParticleFlagsBillboarded | ParticleFlagsNoGravity,
};

struct ParticleEffectDefinition gSpark = {
    .lifetime = 1.0f,
    .fullWidthTime = 0.125f,
    .fadeInEndTime = 0.0f,
    .fadeOutStartTime = 0.5f,
    .tailDelay = 0.05f,
    .minNormalVelocity = -0.5f,
    .maxNormalVelocity = 1.0f,
    .minTangentVelocity = 0.5f,
    .maxTangentVelocity = 1.25f,
    .count = 8,
    .materialIndex = SPARK_INDEX,
    .halfWidth = 0.02f,
    .color = {255, 255, 255, 255},
    .flags = 0,
};

struct ParticleEffectDefinition gSmoke = {
    .lifetime = 2.0f,
    .fullWidthTime = 1.5f,
    .fadeInEndTime = 0.75f,
    .fadeOutStartTime = 1.25f,
    .tailDelay = 2.0f,
    .minNormalVelocity = 0.75f,
    .maxNormalVelocity = 1.25f,
    .minTangentVelocity = 0.0625f,
    .maxTangentVelocity = 0.1875f,
    .count = 2,
    .materialIndex = SMOKE1_INDEX,
    .halfWidth = 2.0f,
    .color = {165, 165, 165, 112},
    .flags = ParticleFlagsBillboarded | ParticleFlagsNoGravity,
};

struct ParticleEffectDefinition gSmokeFast = {
    .lifetime = 1.0f,
    .fullWidthTime = 0.75f,
    .fadeInEndTime = 0.25f,
    .fadeOutStartTime = 0.25f,
    .tailDelay = 0.75f,
    .minNormalVelocity = 2.5f,
    .maxNormalVelocity = 3.0f,
    .minTangentVelocity = 0.0625f,
    .maxTangentVelocity = 0.1875f,
    .count = 2,
    .materialIndex = SMOKE1_INDEX,
    .halfWidth = 2.0f,
    .color = {165, 165, 165, 112},
    .flags = ParticleFlagsBillboarded | ParticleFlagsNoGravity,
};

struct ParticleEffectDefinition* gScriptableEffects[1] = {
    [ScriptableEffectTypeSpark] = &gSpark
};
