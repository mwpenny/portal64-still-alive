#ifndef __EFFECTS_EFFECTS_H__
#define __EFFECTS_EFFECTS_H__

#include "particle_effect.h"

#define MAX_ACTIVE_PARTICLE_EFFECTS 16

struct Effects {
    struct ParticleEffect particleEffects[MAX_ACTIVE_PARTICLE_EFFECTS];
    short nextParticleEffect;
};

void effectsInit(struct Effects* effects);
void effectsParticlePlay(
    struct Effects* effects,
    struct ParticleEffectDefinition* definition,
    struct Vector3* origin,
    struct Vector3* normal,
    struct Transform* parent
);
void effectsUpdate(struct Effects* effects);

#endif
