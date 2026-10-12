#include "effects.h"

void effectsInit(struct Effects* effects) {
    for (int i = 0; i < MAX_ACTIVE_PARTICLE_EFFECTS; ++i) {
        particleEffectInit(&effects->particleEffects[i]);
    }

    effects->nextParticleEffect = 0;
}

void effectsParticlePlay(
    struct Effects* effects,
    struct ParticleEffectDefinition* definition,
    struct Vector3* origin,
    struct Vector3* normal,
    struct Transform* parent,
    int roomIndex,
    enum ParticleCullingType cullingType
) {
    particleEffectPlay(
        &effects->particleEffects[effects->nextParticleEffect],
        definition,
        origin,
        normal,
        parent,
        roomIndex,
        cullingType
    );

    effects->nextParticleEffect = (effects->nextParticleEffect + 1) % MAX_ACTIVE_PARTICLE_EFFECTS;
}

void effectsUpdate(struct Effects* effects) {
    for (int i = 0; i < MAX_ACTIVE_PARTICLE_EFFECTS; ++i) {
        particleEffectUpdate(&effects->particleEffects[i]);
    }
}
