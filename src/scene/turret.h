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
    TurretStateGrabbed,
    TurretStateTipped,
    TurretStateClosing,
    TurretStateDying,
    TurretStateDead
};

enum TurretFlags {
    TurretFlagsOpen = (1 << 0),
    TurretFlagsRotating = (1 << 1)
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

    struct ClosingStateData {
        enum TurretState nextState;
        short soundId;
        short subtitleId;
    } closing;
};

struct Turret {
    struct TurretDefinition* definition;
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    struct SKArmature armature;
    struct Laser laser;
    short dynamicId;
    float fizzleTime;

    enum TurretFlags flags;
    enum TurretState state;
    union TurretStateData stateData;
    struct Quaternion targetRotation;
    float rotationSpeed;
    float stateTimer;
    float openAmount;

    SoundId currentSounds[TurretSoundTypeCount];
};

void turretInit(struct Turret* turret, struct TurretDefinition* definition);
void turretUpdate(struct Turret* turret, struct Player* player);

#endif
