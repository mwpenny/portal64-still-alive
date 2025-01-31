#ifndef __LASER_H__
#define __LASER_H__

#include "math/ray.h"
#include "math/vector3.h"

#define LASER_MAX_BEAMS 4

struct LaserBeam {
    struct Ray startPosition;
    struct Vector3 endPosition;
};

struct Laser {
    struct Transform* parent;
    struct Vector3 parentOffset;
    short dynamicId;

    struct LaserBeam beams[LASER_MAX_BEAMS];
    short beamCount;
};

void laserInit(struct Laser* laser, struct Transform* parent, struct Vector3* offset);
void laserUpdate(struct Laser* laser, int currentRoom);
void laserRemove(struct Laser* laser);

#endif
