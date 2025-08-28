#ifndef __EFFECT_DEFINITIONS_H__
#define __EFFECT_DEFINITIONS_H__

#include "splash_particle_effect.h"

extern struct SplashParticleDefinition gFailPortalSplash[2];

extern struct SplashParticleDefinition gBallBurst;
extern struct SplashParticleDefinition gBallBounce;
extern struct SplashParticleDefinition gMuzzleFlash;
extern struct SplashParticleDefinition gSpark;

enum ScriptableEffectType {
    ScriptableEffectTypeSpark
};

extern struct SplashParticleDefinition* gScriptableEffects[1];

#endif