#include "decor_object.h"

#include "audio/soundplayer.h"
#include "physics/collision_scene.h"
#include "scene/dynamic_scene.h"
#include "system/time.h"
#include "util/dynamic_asset_loader.h"
#include "util/memory.h"

#define TIME_TO_FIZZLE         2.0f
#define FIZZLE_TIME_STEP       (FIXED_DELTA_TIME / TIME_TO_FIZZLE)

#define DECOR_COLLISION_LAYERS (COLLISION_LAYERS_TANGIBLE | COLLISION_LAYERS_GRABBABLE | COLLISION_LAYERS_FIZZLER | COLLISION_LAYERS_BLOCK_TURRET_SHOTS)

Gfx* decorBuildFizzleGfx(Gfx* gfxToRender, float fizzleTime, struct RenderState* renderState) {
    if (fizzleTime <= 0.0f) {
        return gfxToRender;
    }

    Gfx* result = renderStateAllocateDLChunk(renderState, 3);

    Gfx* curr = result;

    int fizzleTimeAsInt = (int)(255.0f * fizzleTime);

    if (fizzleTimeAsInt > 255) {
        fizzleTimeAsInt = 255;
    }

    gDPSetPrimColor(curr++, 255, 255, fizzleTimeAsInt, fizzleTimeAsInt, fizzleTimeAsInt, 255 - fizzleTimeAsInt);
    gSPDisplayList(curr++, gfxToRender);
    gSPEndDisplayList(curr++);

    return result;
}

void decorObjectRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct DecorObject* object = (struct DecorObject*)data;

    Mtx* matrix = renderStateRequestMatrices(renderState, 1);

    if (!matrix) {
        return;
    }

    transformToMatrixL(&object->rigidBody.transform, matrix, SCENE_SCALE);

    dynamicRenderListAddDataTouchingPortal(
        renderList, 
        decorBuildFizzleGfx(dynamicAssetModel(object->definition->dynamicModelIndex), object->fizzleTime, renderState), 
        matrix, 
        (object->fizzleTime > 0.0f) ? object->definition->materialIndexFizzled : object->definition->materialIndex, 
        &object->rigidBody.transform.position, 
        NULL,
        object->rigidBody.flags
    );
}

struct DecorObject* decorObjectNew(struct DecorObjectDefinition* definition, struct Transform* at, int room) {
    struct DecorObject* result = malloc(sizeof(struct DecorObject));
    decorObjectInit(result, definition, at, room);
    return result;
}

void decorObjectReset(struct DecorObject* object) {
    object->rigidBody.transform.position = object->originalPosition;
    object->rigidBody.transform.rotation = object->originalRotation;
    object->rigidBody.velocity = gZeroVec;
    object->rigidBody.angularVelocity = gZeroVec;
    object->rigidBody.flags &= ~(RigidBodyFizzled | RigidBodyDisableGravity);
    object->rigidBody.flags |= RigidBodyFlagsGrabbable;
    object->rigidBody.currentRoom = object->originalRoom;
    object->fizzleTime = 0.0f;
    object->collisionObject.collisionLayers = DECOR_COLLISION_LAYERS;
}

void decorObjectInit(struct DecorObject* object, struct DecorObjectDefinition* definition, struct Transform* at, int room) {
    if (definition->colliderType.type != CollisionShapeTypeNone) {
        collisionObjectInit(&object->collisionObject, &definition->colliderType, &object->rigidBody, definition->mass, DECOR_COLLISION_LAYERS);
        collisionSceneAddDynamicObject(&object->collisionObject);
    } else {
        rigidBodyInit(&object->rigidBody, 1.0f, 1.0f);
        object->collisionObject.body = NULL;
    }

    object->definition = definition;

    object->rigidBody.transform = *at;
    object->rigidBody.flags |= RigidBodyFlagsGrabbable;
    object->rigidBody.currentRoom = room;
    object->fizzleTime = 0.0f;

    object->originalPosition = at->position;
    object->originalRotation = at->rotation;
    object->originalRoom = room;

    dynamicAssetModelPreload(definition->dynamicModelIndex);

    if (definition->colliderType.type != CollisionShapeTypeNone) {
        collisionObjectUpdateBB(&object->collisionObject);
    }

    object->dynamicId = dynamicSceneAdd(object, decorObjectRender, &object->rigidBody.transform.position, definition->radius);

    dynamicSceneSetRoomFlags(object->dynamicId, ROOM_FLAG_FROM_INDEX(room));

    object->playingSound = SOUND_ID_NONE;
}

void decorObjectCleanup(struct DecorObject* decorObject) {
    dynamicSceneRemove(decorObject->dynamicId);
    collisionSceneRemoveDynamicObject(&decorObject->collisionObject);
    if ((decorObject->playingSound != SOUND_ID_NONE) && (soundPlayerIsPlaying(decorObject->playingSound))) {
        soundPlayerStop(decorObject->playingSound);
    }
}

void decorObjectDelete(struct DecorObject* decorObject) {
    decorObjectCleanup(decorObject);
    free(decorObject);
}

enum FizzleCheckResult decorObjectUpdateFizzler(struct CollisionObject* collisionObject, float* fizzleTime) {
    enum FizzleCheckResult result = FizzleCheckResultNone;

    if (collisionObject->body && collisionObject->body->flags & RigidBodyFizzled) {
        if (*fizzleTime == 0.0f) {
            vector3Scale(&collisionObject->body->velocity, &collisionObject->body->velocity, 0.25f);

            struct Quaternion randomRotation;
            quatRandom(&randomRotation);
            struct Vector3 randomAngularVelocity;
            quatMultVector(&randomRotation, &gRight, &randomAngularVelocity);

            vector3AddScaled(&collisionObject->body->angularVelocity, &randomAngularVelocity, 0.3f, &collisionObject->body->angularVelocity);

            result = FizzleCheckResultStart;

            collisionObject->body->flags &= ~RigidBodyFlagsGrabbable;
            collisionObject->body->flags |= RigidBodyDisableGravity;
        }

        collisionObject->collisionLayers = 0;

        if (*fizzleTime < 1.0f) {
            *fizzleTime += FIZZLE_TIME_STEP;
        } else {
            result = FizzleCheckResultEnd;
        }
    }

    return result;
}

int decorObjectUpdate(struct DecorObject* decorObject) {
    if (decorObject->collisionObject.flags & COLLISION_OBJECT_PLAYER_STANDING) {
        decorObject->collisionObject.flags &= ~COLLISION_OBJECT_PLAYER_STANDING;
    }

    if (decorObject->playingSound != SOUND_ID_NONE) {
        soundPlayerUpdatePosition(
            decorObject->playingSound, 
            &decorObject->rigidBody.transform.position, 
            &decorObject->rigidBody.velocity
        );
    }

    enum FizzleCheckResult fizzleResult = decorObjectUpdateFizzler(&decorObject->collisionObject, &decorObject->fizzleTime);

    if (fizzleResult == FizzleCheckResultStart) {
        if (decorObject->playingSound != SOUND_ID_NONE) {
            soundPlayerStop(decorObject->playingSound);
            decorObject->playingSound = SOUND_ID_NONE;
        }

        if (decorObject->definition->soundFizzleId != SOUND_ID_NONE) {
            decorObject->playingSound = soundPlayerPlay(decorObject->definition->soundFizzleId, 2.0f, 0.5f, &decorObject->rigidBody.transform.position, &decorObject->rigidBody.velocity, SoundTypeAll);
        }
    } else if (fizzleResult == FizzleCheckResultEnd) {
        if (decorObject->definition->flags & DecorObjectFlagsImportant) {
            decorObjectReset(decorObject);
            dynamicSceneSetRoomFlags(decorObject->dynamicId, ROOM_FLAG_FROM_INDEX(decorObject->rigidBody.currentRoom));
            return 1;
        }

        if (soundPlayerIsPlaying(decorObject->playingSound)) {
            return 1;
        }

        return 0;
    }

    if (decorObject->definition->soundClipId != -1 && decorObject->playingSound == SOUND_ID_NONE && decorObject->fizzleTime == 0.0f && !(decorObject->definition->flags & DecorObjectFlagsMuted)) {
        decorObject->playingSound = soundPlayerPlay(decorObject->definition->soundClipId, decorObject->definition->soundClipVolume, 1.0f, &decorObject->rigidBody.transform.position, &decorObject->rigidBody.velocity, SoundTypeAll);
    }

    dynamicSceneSetRoomFlags(decorObject->dynamicId, ROOM_FLAG_FROM_INDEX(decorObject->rigidBody.currentRoom));

    return 1;
}
