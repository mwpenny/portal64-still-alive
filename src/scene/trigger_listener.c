#include "trigger_listener.h"

#include "../physics/collision_scene.h"
#include "../decor/decor_object_list.h"
#include "../levels/cutscene_runner.h"

#include "./scene.h"

extern struct ColliderTypeData gPlayerColliderData;

#define TRIGGER_TYPE_TO_MASK(type)      (1 << (type))

enum ObjectTriggerType triggerDetermineType(struct CollisionObject* objectEnteringTrigger) {
    if (objectEnteringTrigger->collider == &gPlayerColliderData) {
        return TRIGGER_TYPE_TO_MASK(ObjectTriggerTypePlayer);
    }

    int decorType = decorIdForObjectDefinition((struct DecorObjectDefinition*)objectEnteringTrigger->collider);

    if (decorType == DECOR_TYPE_CUBE || decorType == DECOR_TYPE_CUBE_UNIMPORTANT) {
        return gScene.player.grabConstraint.object == objectEnteringTrigger ? TRIGGER_TYPE_TO_MASK(ObjectTriggerTypeCubeHover) | TRIGGER_TYPE_TO_MASK(ObjectTriggerTypeCube) : TRIGGER_TYPE_TO_MASK(ObjectTriggerTypeCube);
    }

    return ObjectTriggerTypeNone;
}

void triggerTrigger(void* data, struct CollisionObject* objectEnteringTrigger) {
    struct TriggerListener* listener = data;

    struct Vector3 offset;
    vector3Sub(
        &objectEnteringTrigger->body->transform.position, 
        &listener->body.transform.position, 
        &offset
    );

    if (fabsf(offset.x) > listener->collisionData.sideLength.x ||
        fabsf(offset.y) > listener->collisionData.sideLength.y ||
        fabsf(offset.z) > listener->collisionData.sideLength.z) {
        // only trigger when triggering object is contained
        return;
    }

    enum ObjectTriggerType triggerType = triggerDetermineType(objectEnteringTrigger);

    if (triggerType & listener->usedTriggerMask) {
        // an object activating a signal should not sleep
        objectEnteringTrigger->body->sleepFrames = IDLE_SLEEP_FRAMES;
    }

    listener->lastTriggerMask |= triggerType;
}

void triggerInit(struct TriggerListener* listener, struct Trigger* trigger, int triggerIndex) {
    vector3Sub(&trigger->box.max, &trigger->box.min, &listener->collisionData.sideLength);
    vector3Scale(&listener->collisionData.sideLength, &listener->collisionData.sideLength, 0.5f);

    listener->colliderType.type = CollisionShapeTypeBox;
    listener->colliderType.data = &listener->collisionData; 
    listener->colliderType.bounce = 0.0f;
    listener->colliderType.friction = 0.0f;
    listener->colliderType.callbacks = &gCollisionBoxCallbacks;

    collisionObjectInit(&listener->collisionObject, &listener->colliderType, &listener->body, 1.0f, COLLISION_LAYERS_TANGIBLE);
    rigidBodyMarkKinematic(&listener->body);

    vector3Add(&trigger->box.max, &trigger->box.min, &listener->body.transform.position);
    vector3Scale(&listener->body.transform.position, &listener->body.transform.position, 0.5f);
    collisionObjectUpdateBB(&listener->collisionObject);

    listener->collisionObject.trigger = triggerTrigger;
    listener->collisionObject.data = listener;
    listener->trigger = trigger;
    listener->triggerIndex = triggerIndex;
    
    collisionSceneAddDynamicObject(&listener->collisionObject);

    listener->lastTriggerMask = 0;
    listener->usedTriggerMask = 0;

    for (int i = 0; i < trigger->triggerCount; ++i) {
        struct ObjectTriggerInfo* triggerInfo = &trigger->triggers[i];
        listener->usedTriggerMask |= 1 << triggerInfo->objectType;
    }
}

void triggerListenerUpdate(struct TriggerListener* listener) {
    if (!(listener->lastTriggerMask & listener->usedTriggerMask)) {
        return;
    }

    struct Trigger* trigger = listener->trigger;

    for (int i = 0; i < trigger->triggerCount; ++i) {
        struct ObjectTriggerInfo* triggerInfo = &trigger->triggers[i];

        if ((1 << triggerInfo->objectType) & listener->lastTriggerMask) {
            if (triggerInfo->signalIndex != -1) {
                signalsSend(triggerInfo->signalIndex);
            }

            cutsceneTrigger(triggerInfo->cutsceneIndex, listener->triggerIndex + i);
        }
    }

    listener->lastTriggerMask = 0;
}