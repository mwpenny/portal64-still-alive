#ifndef __LASER_H__
#define __LASER_H__

#include "math/ray.h"
#include "math/vector3.h"
#include "physics/rigid_body.h"

#define LASER_MAX_BEAMS 4

struct LaserBeam {
    struct Ray startPosition;
    struct Vector3 endPosition;
};

struct Laser {
    struct RigidBody* parent;
    struct Vector3* parentOffset;
    struct Quaternion* parentRotation;
    short dynamicId;

    struct CollisionObject* lastObjectHit;

    struct LaserBeam beams[LASER_MAX_BEAMS];
    short beamCount;
};

void laserInit(struct Laser* laser, struct RigidBody* parent, struct Vector3* offset, struct Quaternion* rotation);
void laserUpdate(struct Laser* laser);
void laserRemove(struct Laser* laser);

#endif
