#include "particle_effect.h"

#include "math/mathf.h"
#include "math/vector2.h"
#include "physics/collision_scene.h"
#include "physics/config.h"
#include "scene/dynamic_render_list.h"
#include "scene/dynamic_scene.h"
#include "util/frame_time.h"

#define PARTICLE_EFFECT_CULLING_RADIUS 3.0f

static Vtx* particleEffectBuildQuad(
    Vtx* vtx,
    struct Particle* particle,
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

static void particleEffectBuildVerticesBillboarded(
    Vtx* vtx,
    struct ParticleEffect* effect,
    struct Coloru8* color,
    float widthScalar,
    struct Vector3* cameraPosition
) {
    for (int pidx = 0; pidx < effect->definition->count; ++pidx) {
        struct Particle* particle = &effect->particles[pidx];

        struct Vector3 tmp;
        struct Vector3 heightOffset;
        struct Vector3 position[2];

        vector3Sub(&particle->position[0], &particle->position[1], &position[0]);   // Offset
        vector3AddScaled(&particle->position[1], &position[0], 0.5f, &position[1]); // Center

        // Determine camera-facing basis for billboard
        vector3Sub(&position[1], cameraPosition, &tmp);
        vector3Cross(&tmp, &position[0], &particle->widthOffset);
        vector3Scale(&particle->widthOffset, &particle->widthOffset, effect->definition->halfWidth / sqrtf(vector3MagSqrd(&particle->widthOffset)));

        vector3Cross(&tmp, &particle->widthOffset, &heightOffset);
        vector3Scale(&heightOffset, &heightOffset, 0.5f * sqrtf(vector3MagSqrd(&position[0])) / sqrtf(vector3MagSqrd(&heightOffset)));

        // Start/end relative to center
        vector3Sub(&position[1], &heightOffset, &position[0]);
        vector3Add(&position[1], &heightOffset, &position[1]);

        vtx = particleEffectBuildQuad(vtx, particle, position, color, widthScalar);
    }
}

static void particleEffectBuildVertices(Vtx* vtx, struct ParticleEffect* effect, struct Coloru8* color, float widthScalar) {
    for (int pidx = 0; pidx < effect->definition->count; ++pidx) {
        struct Particle* particle = &effect->particles[pidx];

        vtx = particleEffectBuildQuad(vtx, particle, particle->position, color, widthScalar);
    }
}

static Gfx* particleEffectBuildDisplayList(struct RenderState* renderState, struct ParticleEffect* effect, struct Vector3* cameraPosition) {
    float width = 1.0f;
    if (effect->time < effect->definition->fullWidthTime) {
        width = (effect->time + 0.5f) / (effect->definition->fullWidthTime + 0.5f);
    }

    struct Coloru8 color = effect->definition->color;
    if (effect->time < effect->definition->fadeInEndTime) {
        color.a *= effect->time / effect->definition->fadeInEndTime;
    } else if (effect->time > effect->definition->fadeOutStartTime) {
        color.a *= 1.0f - mathfInvLerp(
            effect->definition->fadeOutStartTime,
            effect->definition->lifetime,
            effect->time
        );
    }

    // Build quads
    Vtx* vertices = renderStateRequestVertices(renderState, effect->definition->count * 4);
    if (cameraPosition) {
        particleEffectBuildVerticesBillboarded(vertices, effect, &color, width, cameraPosition);
    } else {
        particleEffectBuildVertices(vertices, effect, &color, width);
    }

    // Render quads (can load 32 vertices/8 quads at once)
    Gfx* displayList = renderStateAllocateDLChunk(
        renderState,
        effect->definition->count + ((effect->definition->count + 7) >> 3) + 1
    );
    Gfx* dl = displayList;

    for (int i = 0; i < effect->definition->count; ++i) {
        int relativeVertex = (i << 2) & 0x1f;

        if (relativeVertex == 0) {
            // Load next batch of vertices
            int verticesLeft = MIN(32, (effect->definition->count - i) << 2);
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

static int particleEffectIsCulled(struct ParticleEffect* effect, uint64_t* visibleRooms) {
    if (effect->cullingFlags & ParticleCullingFlagsCrossRooms) {
        effect->cullingFlags |= ParticleCullingFlagsWasSeen;

        if (ROOM_FLAG_FROM_INDEX(effect->roomIndex) & *visibleRooms) {
            effect->cullingFlags |= ParticleCullingFlagsRoomWasSeen;
        } else {
            return 1;
        }
    }

    return 0;
}

static void particleEffectRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct ParticleEffect* effect = (struct ParticleEffect*)data;

    if (particleEffectIsCulled(effect, &renderList->visibleRooms)) {
        return;
    }

    Mtx* matrix = NULL;
    if (effect->parent) {
        matrix = renderStateRequestMatrices(renderState, 1);
        transformToMatrixL(effect->parent, matrix, SCENE_SCALE);
    }

    dynamicRenderListAddData(
        renderList,
        particleEffectBuildDisplayList(renderState, effect, NULL),
        matrix,
        effect->definition->materialIndex,
        effect->position,
        NULL
    );
}

static void particleEffectRenderBillboarded(void* data, struct RenderScene* renderScene, struct Transform* fromView) {
    struct ParticleEffect* effect = (struct ParticleEffect*)data;

    if (particleEffectIsCulled(effect, &renderScene->visibleRooms)) {
        return;
    }

    Gfx* gfx;
    Mtx* matrix = NULL;

    if (effect->parent) {
        // Presence of a parent implies a parent-local particle position,
        // which necessitates a parent-local camera position as well
        struct Vector3 localCamPos;
        transformPointInverseNoScale(effect->parent, &fromView->position, &localCamPos);
        gfx = particleEffectBuildDisplayList(renderScene->renderState, effect, &localCamPos);

        matrix = renderStateRequestMatrices(renderScene->renderState, 1);
        transformToMatrixL(effect->parent, matrix, SCENE_SCALE);
    } else {
        gfx = particleEffectBuildDisplayList(renderScene->renderState, effect, &fromView->position);
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

void particleEffectInit(struct ParticleEffect* effect) {
    effect->definition = NULL;
    effect->dynamicId = INVALID_DYNAMIC_OBJECT;
}

void particleEffectPlay(
    struct ParticleEffect* effect,
    struct ParticleEffectDefinition* definition,
    struct Vector3* origin,
    struct Vector3* normal,
    struct Transform* parent,
    int roomIndex,
    enum ParticleCullingType cullingType
) {
    struct Vector3 right;
    struct Vector3 up;
    vector3Perp(normal, &right);
    vector3Normalize(&right, &right);
    vector3Cross(normal, &right, &up);

    for (int i = 0; i < definition->count; ++i) {
        struct Particle* particle = &effect->particles[i];

        // Compute initial velocity and position
        struct Vector2 tangentDir;
        vector2RandomUnitCircle(&tangentDir);
        float tangentMag = randomInRangef(definition->minTangentVelocity, definition->maxTangentVelocity);
        float normalMag = randomInRangef(definition->minNormalVelocity, definition->maxNormalVelocity);

        vector3Scale(normal, &particle->velocity, normalMag);
        vector3AddScaled(&particle->velocity, &right, tangentDir.x * tangentMag, &particle->velocity);
        vector3AddScaled(&particle->velocity, &up, tangentDir.y * tangentMag, &particle->velocity);

        particle->position[1] = *origin;
        vector3AddScaled(&particle->position[1], &particle->velocity, definition->tailDelay, &particle->position[0]);

        // Compute width direction (billboarded particles do this every frame)
        if (!(definition->flags & ParticleFlagsBillboarded)) {
            vector3Cross(&particle->velocity, &gUp, &particle->widthOffset);

            float widthMag = vector3MagSqrd(&particle->widthOffset);
            if (widthMag < 0.00001f) {
                vector3Scale(&gRight, &particle->widthOffset, definition->halfWidth);
            } else {
                vector3Scale(&particle->widthOffset, &particle->widthOffset, definition->halfWidth / sqrtf(widthMag));
            }
        }
    }

    effect->definition = definition;
    effect->time = 0.0f;
    effect->parent = parent;

    if (effect->parent) {
        effect->startPosition = effect->parent->position;
        effect->position = &effect->parent->position;
    } else {
        effect->startPosition = *origin;
        effect->position = &effect->startPosition;
    }

    if (effect->dynamicId != INVALID_DYNAMIC_OBJECT) {
        dynamicSceneRemove(effect->dynamicId);
    }

    if (effect->definition->flags & ParticleFlagsBillboarded) {
        effect->dynamicId = dynamicSceneAddViewDependent(
            effect,
            particleEffectRenderBillboarded,
            effect->position,
            PARTICLE_EFFECT_CULLING_RADIUS
        );
    } else {
        effect->dynamicId = dynamicSceneAdd(
            effect,
            particleEffectRender,
            effect->position,
            PARTICLE_EFFECT_CULLING_RADIUS
        );
    }

    effect->roomIndex = roomIndex;
    effect->cullingFlags = 0;

    if (roomIndex != ROOM_NONE) {
        if (cullingType == ParticleCullingTypeStandard) {
            dynamicSceneSetRoomFlags(effect->dynamicId, ROOM_FLAG_FROM_INDEX(roomIndex));
        } else if (cullingType == ParticleCullingTypeCrossRooms) {
            effect->cullingFlags |= ParticleCullingFlagsCrossRooms;
        }
    }
}

static void particleEffectStop(struct ParticleEffect* effect) {
    effect->definition = NULL;

    dynamicSceneRemove(effect->dynamicId);
    effect->dynamicId = INVALID_DYNAMIC_OBJECT;
}

static int particleEffectUpdateCulling(struct ParticleEffect* effect) {
    if (effect->cullingFlags & ParticleCullingFlagsCrossRooms) {
        if (effect->cullingFlags & ParticleCullingFlagsRoomWasSeen) {
            // Visible from all rooms now (avoids pop out). Don't keep checking.
            effect->cullingFlags &= ~ParticleCullingFlagsCrossRooms;
        } else if (effect->cullingFlags & ParticleCullingFlagsWasSeen) {
            // Room was not visible on first viewing.
            // If the room continues to be hidden, no point showing the effect.
            // If the room becomes visible, the effect will pop in abruptly.
            // Stop to avoid performance impact or jarring graphics.
            return 0;
        } else if (effect->parent) {
            // Ensure accurate room for first viewing check
            effect->roomIndex = worldCheckDoorwayCrossings(
                gCollisionScene.world,
                &effect->startPosition,
                effect->position,
                effect->roomIndex
            );
            effect->startPosition = *effect->position;
        }
    }

    return 1;
}

void particleEffectUpdate(struct ParticleEffect* effect) {
    if (!effect->definition) {
        return;
    }

    if (!particleEffectUpdateCulling(effect)) {
        particleEffectStop(effect);
        return;
    }

    for (int i = 0; i < effect->definition->count; ++i) {
        struct Particle* particle = &effect->particles[i];

        vector3AddScaled(&particle->position[0], &particle->velocity, FIXED_DELTA_TIME, &particle->position[0]);
        vector3AddScaled(&particle->position[1], &particle->velocity, FIXED_DELTA_TIME, &particle->position[1]);

        if (!(effect->definition->flags & ParticleFlagsNoGravity)) {
            // This line simulates tracking the y-velocity of the tail
            // separately without needing to actually do so.
            //
            // tailYVelocity = yVelocity - effect->definition->tailDelay * GRAVITY_CONSTANT
            // tailPos.y = tailPos.y + tailYVelocity * FIXED_DELTA_TIME
            // tailPos.y = tailPos.y + (yVelocity - effect->definition->tailDelay * GRAVITY_CONSTANT) * FIXED_DELTA_TIME
            // tailPos.y = tailPos.y + yVelocity * FIXED_DELTA_TIME - effect->definition->tailDelay * GRAVITY_CONSTANT * FIXED_DELTA_TIME
            particle->position[1].y -= effect->definition->tailDelay * (GRAVITY_CONSTANT * FIXED_DELTA_TIME);

            particle->velocity.y += FIXED_DELTA_TIME * GRAVITY_CONSTANT;
        }
    }

    effect->time += FIXED_DELTA_TIME;

    if (effect->time >= effect->definition->lifetime) {
        particleEffectStop(effect);
    }
}
