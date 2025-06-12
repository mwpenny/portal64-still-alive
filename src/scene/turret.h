#ifndef __TURRET_H__
#define __TURRET_H__

#include <stdint.h>

#include "audio/soundplayer.h"
#include "levels/level_definition.h"
#include "physics/collision_object.h"
#include "physics/compound_collider.h"
#include "player/player.h"
#include "scene/laser.h"

enum TurretState {
    TurretStateIdle,
    TurretStateDeploying,
    TurretStateSearching,
    TurretStateAttacking,
    TurretStateGrabbed,
    TurretStateTipped,
    TurretStateClosing,
    TurretStateDying,
    TurretStateDead
};

enum TurretFlags {
    TurretFlagsOpen             = (1 << 0),
    TurretFlagsRotating         = (1 << 1),
    TurretFlagsShooting         = (1 << 2),
    TurretFlagsTipped           = (1 << 3),
    TurretFlagsPlayerCanAutotip = (1 << 4),
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
        uint8_t yawDirection    : 1;
        uint8_t pitchDirection  : 1;
    } searching;

    struct GrabbedStateData {
        float yawAmount;
        float pitchAmount;
    } grabbed;

    struct ClosingStateData {
        enum TurretState nextState;
        short soundId;
        short subtitleId;
        float nextStateTimer;
    } closing;
};

struct Turret {
    struct CompoundCollider compoundCollider;
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
    uint8_t playerHitCount;
    float rotationSpeed;
    float stateTimer;
    float openAmount;
    float shootTimer;
    float playerDetectTimer;

    SoundId currentSounds[TurretSoundTypeCount];
};

void turretInit(struct Turret* turret, struct TurretDefinition* definition);
int turretUpdate(struct Turret* turret, struct Player* player);

struct Turret* turretNew(struct TurretDefinition* definition);
void turretDelete(struct Turret* turret);

// For serialization
struct Quaternion* turretGetLookRotation(struct Turret* turret);
void turretOnDeserialize(struct Turret* turret);

#endif
