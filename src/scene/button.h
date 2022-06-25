#ifndef __SCENE_BUTTON_H__
#define __SCENE_BUTTON_H__

#include "../physics/collision_object.h"
#include "signals.h"
#include "../levels/level_definition.h"

struct Button {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    short dynamicId;
    short signalIndex;
    short cubeSignalIndex;
    struct Vector3 originalPos;
};

void buttonInit(struct Button* button, struct ButtonDefinition* definition);
void buttonUpdate(struct Button* button);

#endif