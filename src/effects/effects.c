#include "effects.h"


void effectsInit(struct Effects* effects) {
    for (int i = 0; i < MAX_ACTIVE_SPLASH_EFFECTS; ++i) {
        splashParticleEffectInit(&effects->splashParticleEffects[i]);
    }

    effects->nextSplashEffect = 0;
}

void effectsUpdate(struct Effects* effects) {
    for (int i = 0; i < MAX_ACTIVE_SPLASH_EFFECTS; ++i) {
        splashParticleEffectUpdate(&effects->splashParticleEffects[i]);
    }
}

void effectsSplashPlay(struct Effects* effects, struct SplashParticleDefinition* definition, struct Vector3* origin, struct Vector3* normal) {
    splashParticleEffectPlay(&effects->splashParticleEffects[effects->nextSplashEffect], definition, origin, normal);

    ++effects->nextSplashEffect;

    if (effects->nextSplashEffect == MAX_ACTIVE_SPLASH_EFFECTS) {
        effects->nextSplashEffect = 0;
    }
}