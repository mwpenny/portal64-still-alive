#include "fail_portal_splash.h"

#include "../build/assets/materials/static.h"

struct SplashParticleDefinition gFailPortalSplash = {
    .particleLifetime = 1.0f,
    .particleTailDelay = 0.1f,
    .minNormalVelocity = 2.0f,
    .maxNormalVelocity = 3.0f,
    .minTangentVelocity = 3.0f,
    .maxTangentVelocity = 4.0f,
    .particleCount = 15,
    .materialIndex = PORTAL_1_PARTICLE_INDEX,
    .particleHalfWidth = 0.05f,
};