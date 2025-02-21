#ifndef __TURRET_H__
#define __TURRET_H__

#include "audio/soundplayer.h"
#include "levels/level_definition.h"
#include "physics/collision_object.h"
#include "player/player.h"
#include "scene/laser.h"

enum TurretState {
    TurretStateIdle,
    TurretStateSearching,
    TurretStateGrabbed
};

enum TurretFlags {
    TurretFlagsRotating = (1 << 0)
};

enum TurretSoundType {
    TurretSoundTypeDialog,
    TurretSoundTypeSfx,
    TurretSoundTypeCount,
};

union TurretStateData {
    struct SearchingStateData {
        float yawAmount;
        float pitchAmount;
        u8 yawDirection   : 1;
        u8 pitchDirection : 1;
    } searching;

    struct GrabbedStateData {
        float yawAmount;
        float pitchAmount;
    } grabbed;
};

struct Turret {
    struct TurretDefinition* definition;
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    struct SKArmature armature;
    struct Laser laser;
    short dynamicId;
    float fizzleTime;

    enum TurretState state;
    enum TurretFlags flags;
    struct Quaternion targetRotation;
    union TurretStateData stateData;
    float stateTimer;
    float openAmount;

    SoundId currentSounds[TurretSoundTypeCount];
};

void turretInit(struct Turret* turret, struct TurretDefinition* definition);
void turretUpdate(struct Turret* turret, struct Player* player);

#endif
