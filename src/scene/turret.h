#ifndef __TURRET_H__
#define __TURRET_H__

// #include "../physics/collision_object.h"
#include "levels/level_definition.h"
#include "scene/laser.h"
// #include "../sk64/skeletool_armature.h"

struct Turret {
    //struct CollisionObject collisionObject;
    struct TurretDefinition* definition;
    struct RigidBody rigidBody;
    struct SKArmature armature;
    struct Laser laser;
    short dynamicId;
    //float fizzleTime;
};

void turretInit(struct Turret* turret, struct TurretDefinition* definition);
void turretUpdate(struct Turret* turret);

#endif
