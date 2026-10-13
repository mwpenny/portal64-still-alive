#ifndef __LASER_H__
#define __LASER_H__

#include "math/ray.h"
#include "math/vector3.h"
#include "physics/rigid_body.h"

#define LASER_MAX_BEAMS 4

enum LaserFlags {
    LaserFlagsBeamsCalculated = (1 << 0),
};

struct LaserBeam {
    struct Ray startPosition;
    struct Vector3 endPosition;
};

struct Laser {
    struct RigidBody* parent;
    struct Vector3* parentOffset;
    struct Quaternion* parentRotation;
    short dynamicId;

    struct LaserBeam beams[LASER_MAX_BEAMS];
    short beamCount;
    short flags;
};

void laserInit(struct Laser* laser, struct RigidBody* parent, struct Vector3* offset, struct Quaternion* rotation);
void laserUpdate(struct Laser* laser);
void laserRemove(struct Laser* laser);

#endif
