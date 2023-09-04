#ifndef __SCENE_TRIGGER_LISTENER_H__
#define __SCENE_TRIGGER_LISTENER_H__

#include "../physics/collision_object.h"
#include "../physics/collision_box.h"

#include "../levels/level_definition.h"


struct TriggerListener {
    struct CollisionObject collisionObject;
    struct RigidBody body;
    struct ColliderTypeData colliderType;
    struct CollisionBox collisionData;

    struct Trigger* trigger;
    short triggerIndex;
    short lastTriggerMask;
};

void triggerInit(struct TriggerListener* listener, struct Trigger* trigger, int triggerIndex);
void triggerListenerUpdate(struct TriggerListener* listener);

#endif