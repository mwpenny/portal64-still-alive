#ifndef __SCENE_TRIGGER_LISTENER_H__
#define __SCENE_TRIGGER_LISTENER_H__

#include "levels/level_definition.h"
#include "physics/collision_box.h"
#include "physics/collision_object.h"

struct TriggerListener {
    struct CollisionObject collisionObject;
    struct RigidBody body;
    struct ColliderTypeData colliderType;
    struct CollisionBox collisionData;

    struct Trigger* trigger;
    short triggerIndex;
    short lastTriggerMask;
    short usedTriggerMask;
};

void triggerInit(struct TriggerListener* listener, struct Trigger* trigger, int triggerIndex);
void triggerListenerUpdate(struct TriggerListener* listener);

#endif