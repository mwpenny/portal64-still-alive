#include "decor_object.h"

#include "../physics/collision_scene.h"
#include "../scene/dynamic_scene.h"
#include "../util/memory.h"
#include "../audio/soundplayer.h"
#include "../util/time.h"

#define TIME_TO_FIZZLE      2.0f
#define FIZZLE_TIME_STEP    (FIXED_DELTA_TIME / TIME_TO_FIZZLE)

void decorObjectRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct DecorObject* object = (struct DecorObject*)data;

    Mtx* matrix = renderStateRequestMatrices(renderState, 1);

    if (!matrix) {
        return;
    }

    transformToMatrixL(&object->rigidBody.transform, matrix, SCENE_SCALE);

    Gfx* gfxToRender;
    
    if (object->fizzleTime > 0.0f) {
        gfxToRender = renderStateAllocateDLChunk(renderState, 3);

        Gfx* curr = gfxToRender;

        int fizzleTimeAsInt = (int)(255.0f * object->fizzleTime);

        if (fizzleTimeAsInt > 255) {
            fizzleTimeAsInt = 255;
        }

        gDPSetPrimColor(curr++, 255, 255, fizzleTimeAsInt, fizzleTimeAsInt, fizzleTimeAsInt, 255 - fizzleTimeAsInt);
        gSPDisplayList(curr++, object->definition->graphics);
        gSPEndDisplayList(curr++);
    } else {
        gfxToRender = object->definition->graphics;
    }

    dynamicRenderListAddData(
        renderList, 
        gfxToRender, 
        matrix, 
        (object->fizzleTime > 0.0f) ? object->definition->materialIndexFizzled : object->definition->materialIndex, 
        &object->rigidBody.transform.position, 
        NULL
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
    object->fizzleTime = 0.0f;
    object->rigidBody.flags &= ~(RigidBodyFizzled | RigidBodyDisableGravity);
    object->rigidBody.flags |= RigidBodyFlagsGrabbable;
    object->rigidBody.currentRoom = object->originalRoom;
}

void decorObjectInit(struct DecorObject* object, struct DecorObjectDefinition* definition, struct Transform* at, int room) {
    if (definition->colliderType.type != CollisionShapeTypeNone) {
        collisionObjectInit(&object->collisionObject, &definition->colliderType, &object->rigidBody, definition->mass, COLLISION_LAYERS_TANGIBLE | COLLISION_LAYERS_GRABBABLE | COLLISION_LAYERS_FIZZLER);
        collisionSceneAddDynamicObject(&object->collisionObject);
    } else {
        rigidBodyInit(&object->rigidBody, 1.0f, 1.0f);
    }

    object->definition = definition;

    object->rigidBody.transform = *at;
    object->rigidBody.flags |= RigidBodyFlagsGrabbable;
    object->rigidBody.currentRoom = room;
    object->fizzleTime = 0.0f;

    object->originalPosition = at->position;
    object->originalRotation = at->rotation;
    object->originalRoom = room;

    if (definition->colliderType.type != CollisionShapeTypeNone) {
        collisionObjectUpdateBB(&object->collisionObject);
    }

    object->dynamicId = dynamicSceneAdd(object, decorObjectRender, &object->rigidBody.transform.position, definition->radius);

    dynamicSceneSetRoomFlags(object->dynamicId, ROOM_FLAG_FROM_INDEX(room));

    if (definition->soundClipId != -1) {
        object->playingSound = soundPlayerPlay(definition->soundClipId, 1.0f, 1.0f, &object->rigidBody.transform.position, &object->rigidBody.velocity);
    } else {
        object->playingSound = SOUND_ID_NONE;
    }
}

void decorObjectClenaup(struct DecorObject* decorObject) {
    dynamicSceneRemove(decorObject->dynamicId);
    collisionSceneRemoveDynamicObject(&decorObject->collisionObject);
    if ((decorObject->playingSound != SOUND_ID_NONE) && (soundPlayerIsPlaying(decorObject->playingSound))) {
        soundPlayerStop(decorObject->playingSound);
    }
}

void decorObjectDelete(struct DecorObject* decorObject) {
    decorObjectClenaup(decorObject);
    free(decorObject);
}

int decorObjectUpdate(struct DecorObject* decorObject) {
    if (decorObject->playingSound != SOUND_ID_NONE) {
        soundPlayerUpdatePosition(
            decorObject->playingSound, 
            &decorObject->rigidBody.transform.position, 
            &decorObject->rigidBody.velocity
        );
    }

    if (decorObject->rigidBody.flags & RigidBodyFizzled) {
        if (decorObject->definition->flags & DecorObjectFlagsImportant) {
            decorObjectReset(decorObject);
            dynamicSceneSetRoomFlags(decorObject->dynamicId, ROOM_FLAG_FROM_INDEX(decorObject->rigidBody.currentRoom));
            return 1;
        }

        if (decorObject->fizzleTime == 0.0f) {
            vector3Scale(&decorObject->rigidBody.velocity, &decorObject->rigidBody.velocity, 0.25f);

            struct Quaternion randomRotation;
            quatRandom(&randomRotation);
            struct Vector3 randomAngularVelocity;
            quatMultVector(&randomRotation, &gRight, &randomAngularVelocity);

            vector3AddScaled(&decorObject->rigidBody.angularVelocity, &randomAngularVelocity, 0.6f, &decorObject->rigidBody.angularVelocity);
        }

        decorObject->fizzleTime += FIZZLE_TIME_STEP;
        decorObject->collisionObject.body->flags &= ~RigidBodyFlagsGrabbable;
        decorObject->collisionObject.body->flags |= RigidBodyDisableGravity;

        if (decorObject->fizzleTime > 1.0f) {
            return 0;
        }
    }

    dynamicSceneSetRoomFlags(decorObject->dynamicId, ROOM_FLAG_FROM_INDEX(decorObject->rigidBody.currentRoom));

    return 1;
}