#include "splash_particle_effect.h"

#include "../math/mathf.h"
#include "../util/time.h"
#include "../math/vector2.h"
#include "../physics/config.h"
#include "../scene/dynamic_scene.h"
#include "../defs.h"

void splashParticleEffectBuildVtx(Vtx* vtx, struct SplashParticle* particle, int index) {
    int posIndex = index >> 1;
    int widthSign = index & 0x1;

    struct Vector3 finalPos;

    if (widthSign) {
        vector3Add(&particle->position[posIndex], &particle->widthOffset, &finalPos);
    } else {
        vector3Sub(&particle->position[posIndex], &particle->widthOffset, &finalPos);
    }

    vtx->v.ob[0] = (short)(finalPos.x * SCENE_SCALE);
    vtx->v.ob[1] = (short)(finalPos.y * SCENE_SCALE);
    vtx->v.ob[2] = (short)(finalPos.z * SCENE_SCALE);

    vtx->v.flag = 0;

    vtx->v.tc[0] = posIndex ? 0 : (32 << 5);
    vtx->v.tc[1] = widthSign ? 0 : (32 << 5);

    vtx->v.cn[0] = 255;
    vtx->v.cn[1] = 255;
    vtx->v.cn[2] = 255;
    vtx->v.cn[3] = 255;
}

void splashParticleEffectRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct SplashParticleEffect* effect = (struct SplashParticleEffect*)data;

    Vtx* vertices = renderStateRequestVertices(renderState, effect->def->particleCount * 4);
    Vtx* curr = vertices;

    for (int i = 0; i < effect->def->particleCount; ++i) {
        splashParticleEffectBuildVtx(&curr[0], &effect->particles[i], 0);
        splashParticleEffectBuildVtx(&curr[1], &effect->particles[i], 1);
        splashParticleEffectBuildVtx(&curr[2], &effect->particles[i], 2);
        splashParticleEffectBuildVtx(&curr[3], &effect->particles[i], 3);

        curr += 4;
    }

    Gfx* displayList = renderStateAllocateDLChunk(renderState, effect->def->particleCount + (effect->def->particleCount >> 5) + 1);
    Gfx* dl = displayList;

    for (int i = 0; i < effect->def->particleCount; ++i) {
        int relativeVertex = i & 0x1f;

        if (relativeVertex == 0) {
            int verticesLeft = (effect->def->particleCount - i) << 2;

            if (verticesLeft > 32) {
                verticesLeft = 32;
            }

            gSPVertex(dl++, &vertices[i << 2], verticesLeft, 0);
        }

        gSP2Triangles(
            dl++, 
            relativeVertex, 
            relativeVertex + 1, 
            relativeVertex + 2, 
            0, 
            relativeVertex + 2,
            relativeVertex + 1,
            relativeVertex + 3,
            0
        );
    }

    gSPEndDisplayList(dl++);

    dynamicRenderListAddData(renderList, displayList, NULL, effect->def->materialIndex, &effect->startPosition, NULL);
}

void splashParticleEffectInit(struct SplashParticleEffect* effect) {
    effect->def = NULL;
    effect->dynamicId = INVALID_DYNAMIC_OBJECT;
}

void splashParticleEffectPlay(struct SplashParticleEffect* effect, struct SplashParticleDefinition* definiton, struct Vector3* origin, struct Vector3* normal) {
    if (effect->dynamicId != INVALID_DYNAMIC_OBJECT) {
        dynamicSceneRemove(effect->dynamicId);
    }

    effect->def = definiton;
    effect->time = 0.0f;

    struct Vector3 right;
    struct Vector3 up;

    vector3Perp(normal, &right);
    vector3Normalize(&right, &right);
    vector3Cross(normal, &right, &up);

    for (int i = 0; i < effect->def->particleCount; ++i) {
        struct SplashParticle* particle = &effect->particles[i];

        struct Vector3 initialVelocity;

        struct Vector2 tangentDir;
        vector2RandomUnitCircle(&tangentDir);
        float tangentMag = randomInRangef(definiton->minTangentVelocity, definiton->maxTangentVelocity);
        float normalMag = randomInRangef(definiton->minNormalVelocity, definiton->maxNormalVelocity);

        vector3Scale(normal, &initialVelocity, normalMag);
        vector3AddScaled(&initialVelocity, &right, tangentDir.x * tangentMag, &initialVelocity);
        vector3AddScaled(&initialVelocity, &up, tangentDir.y * tangentMag, &initialVelocity);

        particle->position[1] = *origin;
        vector3AddScaled(origin, &initialVelocity, definiton->particleTailDelay, &particle->position[0]);

        vector3Cross(&initialVelocity, &gUp, &particle->widthOffset);

        float widthMag = vector3MagSqrd(&particle->widthOffset);

        if (widthMag < 0.00001f) {
            vector3Scale(&gRight, &particle->widthOffset, definiton->particleHalfWidth);
            particle->widthOffset = gRight;
        } else {
            vector3Scale(&particle->widthOffset, &particle->widthOffset, definiton->particleHalfWidth / sqrtf(widthMag));
        }
    }

    effect->startPosition = *origin;
    effect->dynamicId = dynamicSceneAdd(effect, splashParticleEffectRender, &effect->startPosition, 10.0f);
}

void splashParticleEffectUpdate(struct SplashParticleEffect* effect) {
    if (!effect->def) {
        return;
    }

    for (int i = 0; i < effect->def->particleCount; ++i) {
        struct SplashParticle* particle = &effect->particles[i];

        vector3AddScaled(&particle->position[0], &particle->velocity, FIXED_DELTA_TIME, &particle->position[0]);
        vector3AddScaled(&particle->position[1], &particle->velocity, FIXED_DELTA_TIME, &particle->position[1]);

        // this line simulates tracking the yvelocity of the tail separate
        // without needing to track that velocity separately
        // tailYVelocity = yVelocity - effect->def->particleTailDelay * GRAVITY_CONSTANT
        // tailPos.y = tailPos.y + tailYVelocity * FIXED_DELTA_TIME
        // tailPos.y = tailPos.y + (yVelocity - effect->def->particleTailDelay * GRAVITY_CONSTANT) * FIXED_DELTA_TIME
        // tailPos.y = tailPos.y + yVelocity * FIXED_DELTA_TIME - effect->def->particleTailDelay * GRAVITY_CONSTANT * FIXED_DELTA_TIME
        particle->position[1].y -= effect->def->particleTailDelay * (GRAVITY_CONSTANT * FIXED_DELTA_TIME);

        particle->velocity.y += FIXED_DELTA_TIME * GRAVITY_CONSTANT;
    }

    effect->time += FIXED_DELTA_TIME;

    if (effect->time >= effect->def->particleLifetime) {
        effect->def = NULL;
        dynamicSceneRemove(effect->dynamicId);
        effect->dynamicId = INVALID_DYNAMIC_OBJECT;
    }
}