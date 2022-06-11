#include "decor_object.h"

#include "../physics/collision_scene.h"
#include "../scene/dynamic_scene.h"
#include "../util/memory.h"
#include "../audio/soundplayer.h"

void decorObjectRender(void* data, struct RenderScene* renderScene) {
    struct DecorObject* object = (struct DecorObject*)data;
    Mtx* matrix = renderStateRequestMatrices(renderScene->renderState, 1);
    transformToMatrixL(&object->rigidBody.transform, matrix, SCENE_SCALE);

    renderSceneAdd(renderScene, object->definition->graphics, matrix, object->definition->materialIndex, &object->rigidBody.transform.position, NULL);
}

struct DecorObject* decorObjectNew(struct DecorObjectDefinition* definition, struct Transform* at, int room) {
    struct DecorObject* result = malloc(sizeof(struct DecorObject));
    decorObjectInit(result, definition, at, room);
    return result;
}

void decorObjectInit(struct DecorObject* object, struct DecorObjectDefinition* definition, struct Transform* at, int room) {
    collisionObjectInit(&object->collisionObject, &definition->colliderType, &object->rigidBody, definition->mass, COLLISION_LAYERS_TANGIBLE | COLLISION_LAYERS_GRABBABLE);
    collisionSceneAddDynamicObject(&object->collisionObject);

    object->rigidBody.transform = *at;
    object->rigidBody.flags |= RigidBodyFlagsGrabbable;
    object->rigidBody.currentRoom = room;
    object->definition = definition;

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
}