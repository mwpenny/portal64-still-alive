#include "splash_particle_effect.h"

#include "math/mathf.h"
#include "math/vector2.h"
#include "physics/config.h"
#include "scene/dynamic_scene.h"
#include "util/frame_time.h"

void splashParticleEffectBuildVerticesBillboarded(Vtx* vtx, struct SplashParticleEffect* effect, struct Coloru8* color, float widthScalar, struct Vector3* cameraPosition) {
    for (short pidx = 0; pidx < effect->def->particleCount; ++pidx) {
        struct SplashParticle* particle = &effect->particles[pidx];

        struct Vector3 tmp;
        struct Vector3 widthOffset, heightOffset;
        struct Vector3 startPosition, endPosition;

        struct Vector3 particleDir;
        vector3Sub(&particle->position[1], &particle->position[0], &particleDir);

        // Determine screen space basis for billboard
        vector3Sub(&particle->position[0], cameraPosition, &tmp);
        vector3Cross(&tmp, &particleDir, &widthOffset);
        vector3Scale(&widthOffset, &widthOffset, effect->def->particleHalfWidth / sqrtf(vector3MagSqrd(&widthOffset)));

        vector3Cross(&tmp, &widthOffset, &heightOffset);
        vector3Scale(&heightOffset, &heightOffset, sqrtf(vector3MagSqrd(&particleDir)) / sqrtf(vector3MagSqrd(&heightOffset)));

        // Start/end relative to center
        vector3AddScaled(&particle->position[0], &particleDir, 0.5f, &tmp);
        vector3Sub(&tmp, &heightOffset, &startPosition);
        vector3Add(&tmp, &heightOffset, &endPosition);

        if (effect->parent) {
            transformPointNoScale(effect->parent, &startPosition, &startPosition);
            transformPointNoScale(effect->parent, &endPosition, &endPosition);
            quatMultVector(&effect->parent->rotation, &widthOffset, &widthOffset);
        }

        for (short i = 0; i < 4; ++i, ++vtx) {
            short posIndex = i >> 1;
            short widthSign = i & 0x1;

            vector3AddScaled(
                posIndex ? &endPosition : &startPosition,
                &widthOffset,
                widthSign ? widthScalar : -widthScalar,
                &tmp
            );

            vtx->v.ob[0] = tmp.x * SCENE_SCALE;
            vtx->v.ob[1] = tmp.y * SCENE_SCALE;
            vtx->v.ob[2] = tmp.z * SCENE_SCALE;

            vtx->v.flag = 0;
            vtx->v.tc[0] = widthSign ? 0 : (32 << 5);
            vtx->v.tc[1] = posIndex ? 0 : (32 << 5);

            vtx->v.cn[0] = color->r;
            vtx->v.cn[1] = color->g;
            vtx->v.cn[2] = color->b;
            vtx->v.cn[3] = color->a;
        }
    }
}

static void splashParticleEffectBuildVertices(Vtx* vtx, struct SplashParticleEffect* effect, struct Coloru8* color, float widthScalar) {
    for (short pidx = 0; pidx < effect->def->particleCount; ++pidx) {
        struct SplashParticle* particle = &effect->particles[pidx];

        for (short i = 0; i < 4; ++i, ++vtx) {
            short posIndex = i >> 1;
            short widthSign = i & 0x1;
            struct Vector3 finalPos;

            vector3AddScaled(
                &particle->position[posIndex],
                &particle->widthOffset,
                widthSign ? widthScalar : -widthScalar,
                &finalPos
            );

            if (effect->parent) {
                transformPointNoScale(effect->parent, &finalPos, &finalPos);
            }

            vtx->v.ob[0] = finalPos.x * SCENE_SCALE;
            vtx->v.ob[1] = finalPos.y * SCENE_SCALE;
            vtx->v.ob[2] = finalPos.z * SCENE_SCALE;

            vtx->v.flag = 0;
            vtx->v.tc[0] = widthSign ? 0 : (32 << 5);
            vtx->v.tc[1] = posIndex ? 0 : (32 << 5);

            vtx->v.cn[0] = color->r;
            vtx->v.cn[1] = color->g;
            vtx->v.cn[2] = color->b;
            vtx->v.cn[3] = color->a;
        }
    }
}

static Gfx* splashParticleEffectBuildDisplayList(struct RenderState* renderState, struct SplashParticleEffect* effect, struct Vector3* cameraPosition) {
    struct Coloru8 color = effect->def->particleColor;
    float width = 1.0f;

    if (effect->time < effect->def->fullWidthTime) {
        width = (effect->time + 0.5f) / (effect->def->fullWidthTime + 0.5f);
    }

    if (effect->time > effect->def->fadeStartTime) {
        int alpha = (int)(255.0f * (effect->def->fadeStartTime - effect->time) / (effect->def->particleLifetime - effect->def->fadeStartTime));
        
        if (alpha > 255) {
            alpha = 255;
        }

        color.a = alpha;
    }

    // Build quads
    Vtx* vertices = renderStateRequestVertices(renderState, effect->def->particleCount * 4);
    if (effect->def->flags & SplashParticleFlagsBillboarded) {
        splashParticleEffectBuildVerticesBillboarded(vertices, effect, &color, width, cameraPosition);
    } else {
        splashParticleEffectBuildVertices(vertices, effect, &color, width);
    }

    // Render quads (can load 32 vertices/8 quads at once)
    Gfx* displayList = renderStateAllocateDLChunk(renderState, effect->def->particleCount + ((effect->def->particleCount + 7) >> 3) + 1);
    Gfx* dl = displayList;

    for (short i = 0; i < effect->def->particleCount; ++i) {
        short relativeVertex = (i << 2) & 0x1f;

        if (relativeVertex == 0) {
            // Load next batch of vertices
            short verticesLeft = (effect->def->particleCount - i) << 2;

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

    return displayList;
}

static void splashParticleEffectRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct SplashParticleEffect* effect = (struct SplashParticleEffect*)data;

    dynamicRenderListAddData(
        renderList,
        splashParticleEffectBuildDisplayList(renderState, effect, NULL),
        NULL,
        effect->def->materialIndex,
        effect->position,
        NULL
    );
}

void splashParticleEffectRenderBillboarded(void* data, struct RenderScene* renderScene, struct Transform* fromView) {
    struct SplashParticleEffect* effect = (struct SplashParticleEffect*)data;

    Gfx* gfx;
    if (effect->parent) {
        // Presence of a parent implies a parent-local particle position,
        // which necessitates a parent-local camera position as well
        struct Vector3 localCamPos;
        transformPointInverseNoScale(effect->parent, &fromView->position, &localCamPos);
        gfx = splashParticleEffectBuildDisplayList(renderScene->renderState, effect, &localCamPos);
    } else {
        gfx = splashParticleEffectBuildDisplayList(renderScene->renderState, effect, &fromView->position);
    }

    renderSceneAdd(
        renderScene,
        gfx,
        NULL,
        effect->def->materialIndex,
        effect->position,
        NULL
    );
}

void splashParticleEffectInit(struct SplashParticleEffect* effect) {
    effect->def = NULL;
    effect->dynamicId = INVALID_DYNAMIC_OBJECT;
}

void splashParticleEffectPlay(struct SplashParticleEffect* effect, struct SplashParticleDefinition* definiton, struct Vector3* origin, struct Vector3* normal, struct Transform* parent) {
    if (effect->dynamicId != INVALID_DYNAMIC_OBJECT) {
        dynamicSceneRemove(effect->dynamicId);
    }

    effect->def = definiton;
    effect->time = 0.0f;
    effect->parent = parent;

    struct Vector3 right;
    struct Vector3 up;

    vector3Perp(normal, &right);
    vector3Normalize(&right, &right);
    vector3Cross(normal, &right, &up);

    for (short i = 0; i < effect->def->particleCount; ++i) {
        struct SplashParticle* particle = &effect->particles[i];

        struct Vector2 tangentDir;
        vector2RandomUnitCircle(&tangentDir);
        float tangentMag = randomInRangef(definiton->minTangentVelocity, definiton->maxTangentVelocity);
        float normalMag = randomInRangef(definiton->minNormalVelocity, definiton->maxNormalVelocity);

        vector3Scale(normal, &particle->velocity, normalMag);
        vector3AddScaled(&particle->velocity, &right, tangentDir.x * tangentMag, &particle->velocity);
        vector3AddScaled(&particle->velocity, &up, tangentDir.y * tangentMag, &particle->velocity);

        particle->position[1] = *origin;
        vector3AddScaled(origin, &particle->velocity, definiton->particleTailDelay, &particle->position[0]);

        vector3Cross(&particle->velocity, &gUp, &particle->widthOffset);

        float widthMag = vector3MagSqrd(&particle->widthOffset);

        if (widthMag < 0.00001f) {
            vector3Scale(&gRight, &particle->widthOffset, definiton->particleHalfWidth);
        } else {
            vector3Scale(&particle->widthOffset, &particle->widthOffset, definiton->particleHalfWidth / sqrtf(widthMag));
        }
    }

    effect->startPosition = *origin;
    effect->position = (effect->parent) ? &effect->parent->position : &effect->startPosition;

    if (effect->def->flags & SplashParticleFlagsBillboarded) {
        effect->dynamicId = dynamicSceneAddViewDependent(
            effect,
            splashParticleEffectRenderBillboarded,
            effect->position,
            3.0f
        );
    } else {
        effect->dynamicId = dynamicSceneAdd(
            effect,
            splashParticleEffectRender,
            effect->position,
            3.0f
        );
    }
}

void splashParticleEffectUpdate(struct SplashParticleEffect* effect) {
    if (!effect->def) {
        return;
    }

    for (short i = 0; i < effect->def->particleCount; ++i) {
        struct SplashParticle* particle = &effect->particles[i];

        vector3AddScaled(&particle->position[0], &particle->velocity, FIXED_DELTA_TIME, &particle->position[0]);
        vector3AddScaled(&particle->position[1], &particle->velocity, FIXED_DELTA_TIME, &particle->position[1]);

        if ((effect->def->flags & SplashParticleFlagsNoGravity) == 0) {
            // this line simulates tracking the yvelocity of the tail separate
            // without needing to track that velocity separately
            // tailYVelocity = yVelocity - effect->def->particleTailDelay * GRAVITY_CONSTANT
            // tailPos.y = tailPos.y + tailYVelocity * FIXED_DELTA_TIME
            // tailPos.y = tailPos.y + (yVelocity - effect->def->particleTailDelay * GRAVITY_CONSTANT) * FIXED_DELTA_TIME
            // tailPos.y = tailPos.y + yVelocity * FIXED_DELTA_TIME - effect->def->particleTailDelay * GRAVITY_CONSTANT * FIXED_DELTA_TIME
            particle->position[1].y -= effect->def->particleTailDelay * (GRAVITY_CONSTANT * FIXED_DELTA_TIME);

            particle->velocity.y += FIXED_DELTA_TIME * GRAVITY_CONSTANT;
        }
    }

    effect->time += FIXED_DELTA_TIME;

    if (effect->time >= effect->def->particleLifetime) {
        effect->def = NULL;
        dynamicSceneRemove(effect->dynamicId);
        effect->dynamicId = INVALID_DYNAMIC_OBJECT;
    }
}