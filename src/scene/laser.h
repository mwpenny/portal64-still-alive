#ifndef __LASER_H__
#define __LASER_H__

#include "math/vector3.h"

struct Laser {
    struct Transform* parent;
    struct Vector3 parentOffset;
    short dynamicId;
};

void laserInit(struct Laser* laser, struct Transform* parent, struct Vector3* offset);
void laserUpdate(struct Laser* laser);
void laserRemove(struct Laser* laser);

#endif
