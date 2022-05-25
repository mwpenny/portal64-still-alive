#ifndef __BUTTON_H__
#define __BUTTON_H__

#include "../physics/collision_object.h"

struct Button {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    short dynamicId;
};

void buttonInit(struct Button* button, struct Vector3* at, int roomIndex);
void buttonUpdate(struct Button* button);

#endif