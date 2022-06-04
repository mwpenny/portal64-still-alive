#ifndef __SCENE_BUTTON_H__
#define __SCENE_BUTTON_H__

#include "../physics/collision_object.h"

struct Button {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    short dynamicId;
    short pressStateCounter;
    struct Vector3 originalPos;
};

void buttonInit(struct Button* button, struct Vector3* at, int roomIndex);
void buttonUpdate(struct Button* button);

#endif