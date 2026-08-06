#ifndef __EFFECT_DEFINITIONS_H__
#define __EFFECT_DEFINITIONS_H__

#include "particle_effect.h"

extern struct ParticleEffectDefinition gFailPortalSplash[2];

extern struct ParticleEffectDefinition gBallBurst;
extern struct ParticleEffectDefinition gBallBounce;
extern struct ParticleEffectDefinition gMuzzleFlash;
extern struct ParticleEffectDefinition gSpark;
extern struct ParticleEffectDefinition gSmoke;
extern struct ParticleEffectDefinition gSmokeFast;

enum ScriptableEffectType {
    ScriptableEffectTypeSpark
};

extern struct ParticleEffectDefinition* gScriptableEffects[1];

#endif
