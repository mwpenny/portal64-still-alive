#ifndef __SCENE_BUTTON_H__
#define __SCENE_BUTTON_H__

#include "../physics/collision_object.h"
#include "signals.h"
#include "../levels/level_definition.h"
#include "../audio/clips.h"
#include "../audio/soundplayer.h"

enum ButtonFlags {
    ButtonFlagsBeingPressed = (1 << 0),
};

struct Button {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    short dynamicId;
    short signalIndex;
    short cubeSignalIndex;
    struct Vector3 originalPos;
    short flags;
};

void buttonInit(struct Button* button, struct ButtonDefinition* definition);
void buttonUpdate(struct Button* button);

#endif