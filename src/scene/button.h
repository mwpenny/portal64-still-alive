#ifndef __SCENE_BUTTON_H__
#define __SCENE_BUTTON_H__

#include "audio/clips.h"
#include "audio/soundplayer.h"
#include "levels/level_definition.h"
#include "physics/collision_object.h"
#include "signals.h"

enum ButtonFlags {
    ButtonFlagsFirstUpdate = (1 << 0),
};

enum ButtonState {
    ButtonStateUnpressed,
    ButtonStatePressed,
    ButtonStatePressedByObject,
};

struct Button {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    struct Vector3 originalPos;
    enum ButtonFlags flags;
    enum ButtonState state;
    short dynamicId;
    short signalIndex;
    short objectSignalIndex;
    short objectPressTimer;
};

void buttonInit(struct Button* button, struct ButtonDefinition* definition);
void buttonUpdate(struct Button* button);

#endif
