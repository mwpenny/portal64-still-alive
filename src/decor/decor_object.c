#include "decor_object.h"

#include "../physics/collision_scene.h"
#include "../scene/dynamic_scene.h"
#include "../util/memory.h"
#include "../audio/soundplayer.h"
#include "../util/time.h"

#define TIME_TO_FIZZLE      2.0f
#define FIZZLE_TIME_STEP    (FIXED_DELTA_TIME / TIME_TO_FIZZLE)

void decorObjectRender(void* data, struct RenderScene* renderScene) {
    struct DecorObject* object = (struct DecorObject*)data;
    Mtx* matrix = renderStateRequestMatrices(renderScene->renderState, 1);
    transformToMatrixL(&object->rigidBody.transform, matrix, SCENE_SCALE);

    Gfx* gfxToRender;
    
    if (object->fizzleTime > 0.0f) {
        gfxToRender = renderStateAllocateDLChunk(renderScene->renderState, 3);

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

    renderSceneAdd(
        renderScene, 
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

void decorObjectInit(struct DecorObject* object, struct DecorObjectDefinition* definition, struct Transform* at, int room) {
    collisionObjectInit(&object->collisionObject, &definition->colliderType, &object->rigidBody, definition->mass, COLLISION_LAYERS_TANGIBLE | COLLISION_LAYERS_GRABBABLE | COLLISION_LAYERS_FIZZLER);
    collisionSceneAddDynamicObject(&object->collisionObject);

    object->rigidBody.transform = *at;
    object->rigidBody.flags |= RigidBodyFlagsGrabbable;
    object->rigidBody.currentRoom = room;
    object->definition = definition;
    object->fizzleTime = 0.0f;

    collisionObjectUpdateBB(&object->collisionObject);

    object->dynamicId = dynamicSceneAdd(object, decorObjectRender, &object->rigidBody.transform, definition->radius);

    if (definition->soundClipId != -1) {
        object->playingSound = soundPlayerPlay(definition->soundClipId, 1.0f, 1.0f, &object->rigidBody.transform.position);
    } else {
        object->playingSound = SOUND_ID_NONE;
    }
}

void decorObjectDelete(struct DecorObject* decorObject) {
    dynamicSceneRemove(decorObject->dynamicId);
    collisionSceneRemoveDynamicObject(&decorObject->collisionObject);
    free(decorObject);
}

void decorObjectUpdate(struct DecorObject* decorObject) {
    if (decorObject->playingSound != SOUND_ID_NONE) {
        soundPlayerUpdatePosition(decorObject->playingSound, &decorObject->rigidBody.transform.position);
    }

    if (decorObject->rigidBody.flags & (RigidBodyIsTouchingPortal | RigidBodyWasTouchingPortal)) {
        dynamicSceneSetFlags(decorObject->dynamicId, DYNAMIC_SCENE_OBJECT_FLAGS_TOUCHING_PORTAL);
    } else {
        dynamicSceneClearFlags(decorObject->dynamicId, DYNAMIC_SCENE_OBJECT_FLAGS_TOUCHING_PORTAL);
    }

    if (decorObject->rigidBody.flags & RigidBodyFizzled) {
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
    }
}