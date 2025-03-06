#ifndef __EFFECTS_EFFECTS_H__
#define __EFFECTS_EFFECTS_H__

#include "splash_particle_effect.h"

#define MAX_ACTIVE_SPLASH_EFFECTS  16

struct Effects {
    struct SplashParticleEffect splashParticleEffects[MAX_ACTIVE_SPLASH_EFFECTS];
    short nextSplashEffect;
};

void effectsInit(struct Effects* effects);
void effectsUpdate(struct Effects* effects);

void effectsSplashPlay(struct Effects* effects, struct SplashParticleDefinition* definition, struct Vector3* origin, struct Vector3* normal);

#endif