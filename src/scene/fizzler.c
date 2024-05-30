#include "fizzler.h"

#include "../util/memory.h"
#include "../graphics/render_scene.h"
#include "../scene/dynamic_scene.h"
#include "../physics/collision_scene.h"
#include "../util/dynamic_asset_loader.h"
#include "../math/mathf.h"
#include "../decor/decor_object_list.h"
#include "signals.h"

#include "../../build/assets/models/dynamic_model_list.h"

#include "../build/assets/materials/static.h"

#define IMAGE_WIDTH     16
#define IMAGE_HEIGHT    64

#define FRAME_HEIGHT    2
#define FRAME_WIDTH     0.125

#define GFX_PER_PARTICLE(particleCount) ((particleCount) + (((particleCount) + 7) >> 3) + 1)

void fizzlerTrigger(void* data, struct CollisionObject* objectEnteringTrigger) {
    struct Fizzler* fizzler = (struct Fizzler*)data;
	
    if (objectEnteringTrigger->body) {
        objectEnteringTrigger->body->flags |= RigidBodyFizzled;
    }

    if (fizzler->cubeSignalIndex != -1) {
        int decorType = decorIdForCollisionObject(objectEnteringTrigger);
        if (decorType == DECOR_TYPE_CUBE || decorType == DECOR_TYPE_CUBE_UNIMPORTANT) {
            signalsSend(fizzler->cubeSignalIndex);
        }
    }
}

struct Transform gRelativeLeft = {
    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.707106781, 0.0f, 0.707106781},
    {1.0f, 1.0f, 1.0f},
};

struct Transform gRelativeRight = {
    {0.0f, 0.0f, 0.0f},
    {0.0f, -0.707106781, 0.0f, 0.707106781},
    {1.0f, 1.0f, 1.0f},
};

void fizzlerRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct Fizzler* fizzler = (struct Fizzler*)data;

    Mtx* matrix = renderStateRequestMatrices(renderState, 1);
    
    if (!matrix) {
        return;
    }
    
    transformToMatrixL(&fizzler->rigidBody.transform, matrix, SCENE_SCALE);

    dynamicRenderListAddData(renderList, fizzler->modelGraphics, matrix, PORTAL_CLEANSER_INDEX, &fizzler->rigidBody.transform.position, NULL);

    int height = fizzler->collisionBox.sideLength.y * 2;
    int rows = (int)(height / FRAME_HEIGHT);
    Mtx* sideMatrices = renderStateRequestMatrices(renderState, rows * 2);

    if (!sideMatrices) {
        return;
    }

    Gfx* sideModel = dynamicAssetModel(PROPS_PORTAL_CLEANSER_DYNAMIC_MODEL);
    struct Transform sideTransform;
    int sideY = (height - FRAME_HEIGHT) / 2;

    for (int i = 0; i < rows; ++i, sideY -= FRAME_HEIGHT) {
        int sideIndex = i * 2;

        gRelativeLeft.position.x = fizzler->collisionBox.sideLength.x;
        gRelativeLeft.position.y = sideY;
        transformConcat(&fizzler->rigidBody.transform, &gRelativeLeft, &sideTransform);
        transformToMatrixL(&sideTransform, &sideMatrices[sideIndex], SCENE_SCALE);
        dynamicRenderListAddData(renderList, sideModel, &sideMatrices[sideIndex], PORTAL_CLEANSER_WALL_INDEX, &fizzler->rigidBody.transform.position, NULL);

        gRelativeRight.position.x = -fizzler->collisionBox.sideLength.x;
        gRelativeRight.position.y = sideY;
        transformConcat(&fizzler->rigidBody.transform, &gRelativeRight, &sideTransform);
        transformToMatrixL(&sideTransform, &sideMatrices[sideIndex + 1], SCENE_SCALE);
        dynamicRenderListAddData(renderList, sideModel, &sideMatrices[sideIndex + 1], PORTAL_CLEANSER_WALL_INDEX, &fizzler->rigidBody.transform.position, NULL);
    }
}

void fizzlerSpawnParticle(struct Fizzler* fizzler, int particleIndex) {
    int x = (particleIndex & 0x1) ? -fizzler->maxExtent : fizzler->maxExtent;
    int y = randomInRange(-fizzler->maxVerticalExtent, fizzler->maxVerticalExtent);

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

void fizzlerInit(struct Fizzler* fizzler, struct Transform* transform, float width, float height, int room, short cubeSignalIndex) {
    fizzler->collisionBox.sideLength.x = width;
    fizzler->collisionBox.sideLength.y = height;
    fizzler->collisionBox.sideLength.z = 0.25f;

    fizzler->colliderType.type = CollisionShapeTypeBox;
    fizzler->colliderType.data = &fizzler->collisionBox;
    fizzler->colliderType.bounce = 0.0f;
    fizzler->colliderType.friction = 0.0f;
    fizzler->colliderType.callbacks = &gCollisionBoxCallbacks;

    fizzler->frameCollisionBox.sideLength.x = FRAME_WIDTH;
    fizzler->frameCollisionBox.sideLength.y = height;
    fizzler->frameCollisionBox.sideLength.z = FRAME_WIDTH;

    fizzler->frameColliderType.type = CollisionShapeTypeBox;
    fizzler->frameColliderType.data = &fizzler->frameCollisionBox;
    fizzler->frameColliderType.bounce = 0.0f;
    fizzler->frameColliderType.friction = 0.0f;
    fizzler->frameColliderType.callbacks = &gCollisionBoxCallbacks;

    collisionObjectInit(&fizzler->collisionObject, &fizzler->colliderType, &fizzler->rigidBody, 1.0f, COLLISION_LAYERS_FIZZLER | COLLISION_LAYERS_BLOCK_PORTAL);
    rigidBodyMarkKinematic(&fizzler->rigidBody);
    collisionObjectInit(&fizzler->frameLeftCollisionObject, &fizzler->frameColliderType, &fizzler->frameLeftRigidBody, 1.0f, COLLISION_LAYERS_TANGIBLE);
    rigidBodyMarkKinematic(&fizzler->frameLeftRigidBody);
    collisionObjectInit(&fizzler->frameRightCollisionObject, &fizzler->frameColliderType, &fizzler->frameRightRigidBody, 1.0f, COLLISION_LAYERS_TANGIBLE);
    rigidBodyMarkKinematic(&fizzler->frameRightRigidBody);

    fizzler->collisionObject.trigger = fizzlerTrigger;
    fizzler->collisionObject.data = fizzler;
    fizzler->rigidBody.transform = *transform;
    fizzler->rigidBody.currentRoom = room;
    
    struct Vector3 left = {-1.0f, 0.0f, 0.0f};
    quatMultVector(&transform->rotation, &left, &left);
    fizzler->frameLeftRigidBody.transform = *transform;
    vector3AddScaled(&transform->position, &left, width - fizzler->frameCollisionBox.sideLength.x, &fizzler->frameLeftRigidBody.transform.position);
    fizzler->frameLeftRigidBody.currentRoom = room;
    
    struct Vector3 right;
    vector3Negate(&left, &right);
    fizzler->frameRightRigidBody.transform = *transform;
    vector3AddScaled(&transform->position, &right, width - fizzler->frameCollisionBox.sideLength.x, &fizzler->frameRightRigidBody.transform.position);
    fizzler->frameRightRigidBody.currentRoom = room;
    
    fizzler->cubeSignalIndex = cubeSignalIndex;
    
    collisionObjectUpdateBB(&fizzler->collisionObject);
    collisionObjectUpdateBB(&fizzler->frameLeftCollisionObject);
    collisionObjectUpdateBB(&fizzler->frameRightCollisionObject);
    
    collisionSceneAddDynamicObject(&fizzler->collisionObject);
    collisionSceneAddDynamicObject(&fizzler->frameLeftCollisionObject);
    collisionSceneAddDynamicObject(&fizzler->frameRightCollisionObject);
    
    fizzler->maxExtent = (int)(maxf(0.0f, width - 0.5f) * SCENE_SCALE);
    fizzler->maxVerticalExtent = (int)(height * SCENE_SCALE);

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
    fizzler->dynamicId = dynamicSceneAdd(fizzler, fizzlerRender, &fizzler->rigidBody.transform.position, sqrtf(width * width + height * height));

    dynamicSceneSetRoomFlags(fizzler->dynamicId, ROOM_FLAG_FROM_INDEX(room));

    dynamicAssetModelPreload(PROPS_PORTAL_CLEANSER_DYNAMIC_MODEL);
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
    
    osWritebackDCache(fizzler->modelVertices, sizeof(Vtx) * maxVertex);
}