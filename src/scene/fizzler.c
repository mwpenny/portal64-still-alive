#include "fizzler.h"

#include "../util/memory.h"
#include "../graphics/render_scene.h"
#include "../scene/dynamic_scene.h"
#include "../physics/collision_scene.h"
#include "../models/models.h"

#define GFX_PER_PARTICLE(particleCount) ((particleCount) + (((particleCount) + 7) >> 3) + 1)

void fizzlerTrigger(void* data, struct CollisionObject* objectEnteringTrigger) {
    if (objectEnteringTrigger->body) {
        objectEnteringTrigger->body->flags |= RigidBodyFizzled;
    }
}

void fizzlerRender(void* data, struct RenderScene* renderScene) {
    struct Fizzler* fizzler = (struct Fizzler*)data;

    Mtx* matrix = renderStateRequestMatrices(renderScene->renderState, 1);
    transformToMatrixL(&fizzler->rigidBody.transform, matrix, SCENE_SCALE);

    renderSceneAdd(renderScene, fizzler->modelGraphics, matrix, fizzler_material_index, &fizzler->rigidBody.transform.position, NULL);
}

void fizzlerSpawnParticle(struct Fizzler* fizzler, int particleIndex) {
    int x = (particleIndex & 0x1) ? -fizzler->maxExtent : fizzler->maxExtent;
    int y = guRandom() % (fizzler->maxVerticalExtent * 2) - fizzler->maxVerticalExtent;

    int xSize = (particleIndex & 0x1) ? (FIZZLER_PARTICLE_LENGTH_FIXED / 2) : -(FIZZLER_PARTICLE_LENGTH_FIXED / 2);

    Vtx* currentVertex = &fizzler->modelVertices[particleIndex << 2];

    currentVertex->v.ob[0] = x - xSize;
    currentVertex->v.ob[1] = y - (FIZZLER_PARTICLE_HEIGHT_FIXED / 2);
    currentVertex->v.ob[2] = 0;

    currentVertex->v.flag = 0;
    currentVertex->v.tc[0] = 0;
    currentVertex->v.tc[1] = 0;

    currentVertex->v.cn[0] = 255; currentVertex->v.cn[1] = 255; currentVertex->v.cn[2] = 255; currentVertex->v.cn[3] = 255;

    ++currentVertex;

    currentVertex->v.ob[0] = x - xSize;
    currentVertex->v.ob[1] = y + (FIZZLER_PARTICLE_HEIGHT_FIXED / 2);
    currentVertex->v.ob[2] = 0;

    currentVertex->v.flag = 0;
    currentVertex->v.tc[0] = IMAGE_WIDTH << 5;
    currentVertex->v.tc[1] = 0;

    currentVertex->v.cn[0] = 255; currentVertex->v.cn[1] = 255; currentVertex->v.cn[2] = 255; currentVertex->v.cn[3] = 255;

    ++currentVertex;

    currentVertex->v.ob[0] = x + xSize;
    currentVertex->v.ob[1] = y + (FIZZLER_PARTICLE_HEIGHT_FIXED / 2);
    currentVertex->v.ob[2] = 0;

    currentVertex->v.flag = 0;
    currentVertex->v.tc[0] = IMAGE_WIDTH << 5;
    currentVertex->v.tc[1] = IMAGE_HEIGHT << 5;

    currentVertex->v.cn[0] = 255; currentVertex->v.cn[1] = 255; currentVertex->v.cn[2] = 255; currentVertex->v.cn[3] = 255;

    ++currentVertex;

    currentVertex->v.ob[0] = x + xSize;
    currentVertex->v.ob[1] = y - (FIZZLER_PARTICLE_HEIGHT_FIXED / 2);
    currentVertex->v.ob[2] = 0;

    currentVertex->v.flag = 0;
    currentVertex->v.tc[0] = 0;
    currentVertex->v.tc[1] = IMAGE_HEIGHT << 5;

    currentVertex->v.cn[0] = 255; currentVertex->v.cn[1] = 255; currentVertex->v.cn[2] = 255; currentVertex->v.cn[3] = 255;
}

void fizzlerInit(struct Fizzler* fizzler, struct Transform* transform, float width, float height, int room) {
    fizzler->collisionBox.sideLength.x = width;
    fizzler->collisionBox.sideLength.y = height;
    fizzler->collisionBox.sideLength.z = 0.25f;

    fizzler->colliderType.type = CollisionShapeTypeBox;
    fizzler->colliderType.data = &fizzler->collisionBox;
    fizzler->colliderType.bounce = 0.0f;
    fizzler->colliderType.friction = 0.0f;
    fizzler->colliderType.callbacks = &gCollisionBoxCallbacks;

    collisionObjectInit(&fizzler->collisionObject, &fizzler->colliderType, &fizzler->rigidBody, 1.0f, COLLISION_LAYERS_FIZZLER | COLLISION_LAYERS_BLOCK_PORTAL);
    rigidBodyMarkKinematic(&fizzler->rigidBody);

    fizzler->collisionObject.trigger = fizzlerTrigger;

    fizzler->rigidBody.transform = *transform;
    fizzler->rigidBody.currentRoom = room;

    collisionObjectUpdateBB(&fizzler->collisionObject);
    collisionSceneAddDynamicObject(&fizzler->collisionObject);

    fizzler->maxExtent = (int)(width * SCENE_SCALE * 0.5f);
    fizzler->maxVerticalExtent = (int)(height * SCENE_SCALE * 0.5f);

    fizzler->particleCount = (int)(width * height * FIZZLER_PARTICLES_PER_1x1);

    fizzler->modelVertices = malloc(fizzler->particleCount * 4 * sizeof(Vtx));
    fizzler->modelGraphics = malloc(GFX_PER_PARTICLE(fizzler->particleCount) * sizeof(Gfx));

    Gfx* curr = fizzler->modelGraphics;

    for (int currentParticle = 0; currentParticle < fizzler->particleCount; currentParticle += 8) {
        int endParticle = currentParticle + 8;

        if (endParticle > fizzler->particleCount) {
            endParticle = fizzler->particleCount;
        }

        int vertexCount = (endParticle - currentParticle) << 2;

        gSPVertex(curr++, &fizzler->modelVertices[currentParticle << 2], vertexCount, 0);

        for (int currentIndex = 0; currentIndex < vertexCount; currentIndex += 4) {
            gSP2Triangles(curr++, 
                currentIndex + 0, currentIndex + 1, currentIndex + 2, 0,
                currentIndex + 0, currentIndex + 2, currentIndex + 3, 0
            );
        }
    }

    gSPEndDisplayList(curr++);

    for (int i = 0; i < fizzler->particleCount; ++i) {
        fizzlerSpawnParticle(fizzler, i);

        int offset = fizzler->maxExtent * 2 * (fizzler->particleCount - i) / fizzler->particleCount;

        if (!(i & 0x1)) {
            offset = -offset;
        }

        int maxVertex = (i + 1) << 2;
        for (int currVertex = (i << 2); currVertex < maxVertex; ++currVertex) {
            fizzler->modelVertices[currVertex].v.ob[0] += offset;
        }
    }

    fizzler->oldestParticleIndex = 0;
    fizzler->dynamicId = dynamicSceneAdd(fizzler, fizzlerRender, &fizzler->rigidBody.transform, sqrtf(width * width + height * height) * 0.5f);
}

void fizzlerUpdate(struct Fizzler* fizzler) {
    Vtx* currentVertex = fizzler->modelVertices;

    int maxVertex = fizzler->particleCount << 2;

    for (int vertexIndex = 0; vertexIndex < maxVertex; ++vertexIndex) {
        int delta = (vertexIndex & 0x4) ? FIZZLER_UNITS_PER_UPDATE : -FIZZLER_UNITS_PER_UPDATE;
        currentVertex->v.ob[0] += delta;
        ++currentVertex;
    }

    if ((fizzler->oldestParticleIndex & 0x1) ? fizzler->modelVertices[fizzler->oldestParticleIndex << 2].v.ob[0] > fizzler->maxExtent : fizzler->modelVertices[fizzler->oldestParticleIndex << 2].v.ob[0] < -fizzler->maxExtent) {
        fizzlerSpawnParticle(fizzler, fizzler->oldestParticleIndex);

        ++fizzler->oldestParticleIndex;

        if (fizzler->oldestParticleIndex == fizzler->particleCount) {
            fizzler->oldestParticleIndex = 0;
        }
    }
}