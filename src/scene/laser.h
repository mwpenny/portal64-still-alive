#ifndef __LASER_H__
#define __LASER_H__

#include "math/vector3.h"

struct Laser {
    struct Transform* parent;
    struct Vector3 parentOffset;
    short dynamicId;

    struct Vector3 startPosition;
    struct Vector3 endPosition;
};

void laserInit(struct Laser* laser, struct Transform* parent, struct Vector3* offset);
void laserUpdate(struct Laser* laser, int currentRoom);
void laserRemove(struct Laser* laser);

#endif
