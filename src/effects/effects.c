#include "effects.h"


void effectsInit(struct Effects* effects) {
    for (int i = 0; i < MAX_ACTIVE_SPLASH_EFFECTS; ++i) {
        splashParticleEffectInit(&effects->splashParticleEffects[i]);
    }

    effects->nextSplashEffect = 0;
}

void effectsSplashPlay(
    struct Effects* effects,
    struct SplashParticleDefinition* definition,
    struct Vector3* origin,
    struct Vector3* normal,
    struct Transform* parent
) {
    splashParticleEffectPlay(
        &effects->splashParticleEffects[effects->nextSplashEffect],
        definition,
        origin,
        normal,
        parent
    );

    effects->nextSplashEffect = (effects->nextSplashEffect + 1) % MAX_ACTIVE_SPLASH_EFFECTS;
}

void effectsUpdate(struct Effects* effects) {
    for (int i = 0; i < MAX_ACTIVE_SPLASH_EFFECTS; ++i) {
        splashParticleEffectUpdate(&effects->splashParticleEffects[i]);
    }
}
