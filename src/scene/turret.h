#ifndef __TURRET_H__
#define __TURRET_H__

#include "audio/soundplayer.h"
#include "levels/level_definition.h"
#include "physics/collision_object.h"
#include "scene/laser.h"

struct Turret {
    struct TurretDefinition* definition;
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    struct SKArmature armature;
    struct Laser laser;
    short dynamicId;
    float fizzleTime;
    SoundId currentSound;
};

void turretInit(struct Turret* turret, struct TurretDefinition* definition);
void turretUpdate(struct Turret* turret);

#endif
