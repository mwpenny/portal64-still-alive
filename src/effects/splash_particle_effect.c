#include "splash_particle_effect.h"

#include "math/mathf.h"
#include "math/vector2.h"
#include "physics/config.h"
#include "scene/dynamic_scene.h"
#include "util/frame_time.h"

static Vtx* splashParticleEffectBuildQuad(
    Vtx* vtx,
    struct SplashParticle* particle,
    struct Vector3* position,
    struct Coloru8* color,
    float widthScalar
) {
    for (int i = 0; i < 4; ++i, ++vtx) {
        int posIndex = i >> 1;
        int widthSign = i & 0x1;
        struct Vector3 finalPos;

        vector3AddScaled(
            &position[posIndex],
            &particle->widthOffset,
            widthSign ? widthScalar : -widthScalar,
            &finalPos
        );

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

    return vtx;
}

static void splashParticleEffectBuildVerticesBillboarded(
    Vtx* vtx,
    struct SplashParticleEffect* effect,
    struct Coloru8* color,
    float widthScalar,
    struct Vector3* cameraPosition
) {
    for (int pidx = 0; pidx < effect->definition->particleCount; ++pidx) {
        struct SplashParticle* particle = &effect->particles[pidx];

        struct Vector3 tmp;
        struct Vector3 heightOffset;
        struct Vector3 position[2];

        vector3Sub(&particle->position[0], &particle->position[1], &position[0]);   // Offset
        vector3AddScaled(&particle->position[1], &position[0], 0.5f, &position[1]); // Center

        // Determine camera-facing basis for billboard
        vector3Sub(&position[1], cameraPosition, &tmp);
        vector3Cross(&tmp, &position[0], &particle->widthOffset);
        vector3Scale(&particle->widthOffset, &particle->widthOffset, effect->definition->particleHalfWidth / sqrtf(vector3MagSqrd(&particle->widthOffset)));

        vector3Cross(&tmp, &particle->widthOffset, &heightOffset);
        vector3Scale(&heightOffset, &heightOffset, 0.5f * sqrtf(vector3MagSqrd(&position[0])) / sqrtf(vector3MagSqrd(&heightOffset)));

        // Start/end relative to center
        vector3Sub(&position[1], &heightOffset, &position[0]);
        vector3Add(&position[1], &heightOffset, &position[1]);

        vtx = splashParticleEffectBuildQuad(vtx, particle, position, color, widthScalar);
    }
}

static void splashParticleEffectBuildVertices(Vtx* vtx, struct SplashParticleEffect* effect, struct Coloru8* color, float widthScalar) {
    for (int pidx = 0; pidx < effect->definition->particleCount; ++pidx) {
        struct SplashParticle* particle = &effect->particles[pidx];

        vtx = splashParticleEffectBuildQuad(vtx, particle, particle->position, color, widthScalar);
    }
}

static Gfx* splashParticleEffectBuildDisplayList(struct RenderState* renderState, struct SplashParticleEffect* effect, struct Vector3* cameraPosition) {
    float width = 1.0f;
    if (effect->time < effect->definition->fullWidthTime) {
        width = (effect->time + 0.5f) / (effect->definition->fullWidthTime + 0.5f);
    }

    struct Coloru8 color = effect->definition->particleColor;
    if (effect->time < effect->definition->fadeInEndTime) {
        color.a *= effect->time / effect->definition->fadeInEndTime;
    } else if (effect->time > effect->definition->fadeOutStartTime) {
        color.a *= 1.0f - mathfInvLerp(
            effect->definition->fadeOutStartTime,
            effect->definition->particleLifetime,
            effect->time
        );
    }

    // Build quads
    Vtx* vertices = renderStateRequestVertices(renderState, effect->definition->particleCount * 4);
    if (cameraPosition) {
        splashParticleEffectBuildVerticesBillboarded(vertices, effect, &color, width, cameraPosition);
    } else {
        splashParticleEffectBuildVertices(vertices, effect, &color, width);
    }

    // Render quads (can load 32 vertices/8 quads at once)
    Gfx* displayList = renderStateAllocateDLChunk(
        renderState,
        effect->definition->particleCount + ((effect->definition->particleCount + 7) >> 3) + 1
    );
    Gfx* dl = displayList;

    for (int i = 0; i < effect->definition->particleCount; ++i) {
        int relativeVertex = (i << 2) & 0x1f;

        if (relativeVertex == 0) {
            // Load next batch of vertices
            int verticesLeft = MIN(32, (effect->definition->particleCount - i) << 2);
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

    Mtx* matrix = NULL;
    if (effect->parent) {
        matrix = renderStateRequestMatrices(renderState, 1);
        transformToMatrixL(effect->parent, matrix, SCENE_SCALE);
    }

    dynamicRenderListAddData(
        renderList,
        splashParticleEffectBuildDisplayList(renderState, effect, NULL),
        matrix,
        effect->definition->materialIndex,
        effect->position,
        NULL
    );
}

static void splashParticleEffectRenderBillboarded(void* data, struct RenderScene* renderScene, struct Transform* fromView) {
    struct SplashParticleEffect* effect = (struct SplashParticleEffect*)data;

    Gfx* gfx;
    Mtx* matrix = NULL;

    if (effect->parent) {
        // Presence of a parent implies a parent-local particle position,
        // which necessitates a parent-local camera position as well
        struct Vector3 localCamPos;
        transformPointInverseNoScale(effect->parent, &fromView->position, &localCamPos);
        gfx = splashParticleEffectBuildDisplayList(renderScene->renderState, effect, &localCamPos);

        matrix = renderStateRequestMatrices(renderScene->renderState, 1);
        transformToMatrixL(effect->parent, matrix, SCENE_SCALE);
    } else {
        gfx = splashParticleEffectBuildDisplayList(renderScene->renderState, effect, &fromView->position);
    }

    renderSceneAdd(
        renderScene,
        gfx,
        matrix,
        effect->definition->materialIndex,
        effect->position,
        NULL
    );
}

void splashParticleEffectInit(struct SplashParticleEffect* effect) {
    effect->definition = NULL;
    effect->dynamicId = INVALID_DYNAMIC_OBJECT;
}

void splashParticleEffectPlay(
    struct SplashParticleEffect* effect,
    struct SplashParticleDefinition* definition,
    struct Vector3* origin,
    struct Vector3* normal,
    struct Transform* parent
) {
    struct Vector3 right;
    struct Vector3 up;
    vector3Perp(normal, &right);
    vector3Normalize(&right, &right);
    vector3Cross(normal, &right, &up);

    for (int i = 0; i < definition->particleCount; ++i) {
        struct SplashParticle* particle = &effect->particles[i];

        // Compute initial velocity and position
        struct Vector2 tangentDir;
        vector2RandomUnitCircle(&tangentDir);
        float tangentMag = randomInRangef(definition->minTangentVelocity, definition->maxTangentVelocity);
        float normalMag = randomInRangef(definition->minNormalVelocity, definition->maxNormalVelocity);

        vector3Scale(normal, &particle->velocity, normalMag);
        vector3AddScaled(&particle->velocity, &right, tangentDir.x * tangentMag, &particle->velocity);
        vector3AddScaled(&particle->velocity, &up, tangentDir.y * tangentMag, &particle->velocity);

        particle->position[1] = *origin;
        vector3AddScaled(&particle->position[1], &particle->velocity, definition->particleTailDelay, &particle->position[0]);

        // Compute width direction (billboarded particles do this every frame)
        if (!(definition->flags & SplashParticleFlagsBillboarded)) {
            vector3Cross(&particle->velocity, &gUp, &particle->widthOffset);

            float widthMag = vector3MagSqrd(&particle->widthOffset);
            if (widthMag < 0.00001f) {
                vector3Scale(&gRight, &particle->widthOffset, definition->particleHalfWidth);
            } else {
                vector3Scale(&particle->widthOffset, &particle->widthOffset, definition->particleHalfWidth / sqrtf(widthMag));
            }
        }
    }

    effect->definition = definition;
    effect->time = 0.0f;
    effect->parent = parent;
    effect->startPosition = *origin;
    effect->position = (effect->parent) ? &effect->parent->position : &effect->startPosition;

    if (effect->dynamicId != INVALID_DYNAMIC_OBJECT) {
        dynamicSceneRemove(effect->dynamicId);
    }

    if (effect->definition->flags & SplashParticleFlagsBillboarded) {
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
    if (!effect->definition) {
        return;
    }

    for (int i = 0; i < effect->definition->particleCount; ++i) {
        struct SplashParticle* particle = &effect->particles[i];

        vector3AddScaled(&particle->position[0], &particle->velocity, FIXED_DELTA_TIME, &particle->position[0]);
        vector3AddScaled(&particle->position[1], &particle->velocity, FIXED_DELTA_TIME, &particle->position[1]);

        if (!(effect->definition->flags & SplashParticleFlagsNoGravity)) {
            // This line simulates tracking the y-velocity of the tail
            // separately without needing to actually do so.
            //
            // tailYVelocity = yVelocity - effect->definition->particleTailDelay * GRAVITY_CONSTANT
            // tailPos.y = tailPos.y + tailYVelocity * FIXED_DELTA_TIME
            // tailPos.y = tailPos.y + (yVelocity - effect->definition->particleTailDelay * GRAVITY_CONSTANT) * FIXED_DELTA_TIME
            // tailPos.y = tailPos.y + yVelocity * FIXED_DELTA_TIME - effect->definition->particleTailDelay * GRAVITY_CONSTANT * FIXED_DELTA_TIME
            particle->position[1].y -= effect->definition->particleTailDelay * (GRAVITY_CONSTANT * FIXED_DELTA_TIME);

            particle->velocity.y += FIXED_DELTA_TIME * GRAVITY_CONSTANT;
        }
    }

    effect->time += FIXED_DELTA_TIME;

    if (effect->time >= effect->definition->particleLifetime) {
        effect->definition = NULL;

        dynamicSceneRemove(effect->dynamicId);
        effect->dynamicId = INVALID_DYNAMIC_OBJECT;
    }
}
