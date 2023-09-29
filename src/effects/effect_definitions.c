#include "effect_definitions.h"

#include "../build/assets/materials/static.h"

struct SplashParticleDefinition gFailPortalSplash[2] = {
    {
        .particleLifetime = 0.5f,
        .fullWidthTime = 0.125f,
        .fadeStartTime = 0.25f,
        .particleTailDelay = 0.1f,
        .minNormalVelocity = 0.5f,
        .maxNormalVelocity = 1.0f,
        .minTangentVelocity = 1.0f,
        .maxTangentVelocity = 2.0f,
        .particleCount = 16,
        .materialIndex = PORTAL_1_PARTICLE_INDEX,
        .particleHalfWidth = 0.05f,
        .particleColor = {200, 100, 50, 255},
    },
    {
        .particleLifetime = 0.5f,
        .fullWidthTime = 0.125f,
        .fadeStartTime = 0.25f,
        .particleTailDelay = 0.1f,
        .minNormalVelocity = 0.5f,
        .maxNormalVelocity = 1.0f,
        .minTangentVelocity = 1.0f,
        .maxTangentVelocity = 2.0f,
        .particleCount = 16,
        .materialIndex = PORTAL_1_PARTICLE_INDEX,
        .particleHalfWidth = 0.05f,
        .particleColor = {50, 70, 200, 255},
    },
};

struct SplashParticleDefinition gBallBurst = {
    .particleLifetime = 2.0f,
    .fullWidthTime = 0.125f,
    .fadeStartTime = 1.5f,
    .particleTailDelay = 0.1f,
    .minNormalVelocity = -1.0f,
    .maxNormalVelocity = 6.0f,
    .minTangentVelocity = 0.5f,
    .maxTangentVelocity = 1.0f,
    .particleCount = 16,
    .materialIndex = SPARK_INDEX,
    .particleHalfWidth = 0.02f,
    .particleColor = {255, 255, 255, 255},
};


struct SplashParticleDefinition gBallBounce = {
    .particleLifetime = 0.75f,
    .fullWidthTime = 0.125f,
    .fadeStartTime = 0.5f,
    .particleTailDelay = 0.1f,
    .minNormalVelocity = 0.5f,
    .maxNormalVelocity = 1.5f,
    .minTangentVelocity = 0.5f,
    .maxTangentVelocity = 1.0f,
    .particleCount = 16,
    .materialIndex = SPARK_INDEX,
    .particleHalfWidth = 0.02f,
    .particleColor = {255, 255, 255, 255},
};