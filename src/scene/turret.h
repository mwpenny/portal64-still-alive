#ifndef __TURRET_H__
#define __TURRET_H__

// #include "../physics/collision_object.h"
#include "levels/level_definition.h"
// #include "../sk64/skeletool_armature.h"

struct Turret {
    //struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    struct SKArmature armature;
    short dynamicId;
    //float fizzleTime;
};

void turretInit(struct Turret* turret, struct TurretDefinition* definition);
void turretUpdate(struct Turret* turret);

#endif
