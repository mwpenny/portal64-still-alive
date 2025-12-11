#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "graphics/renderstate.h"
#include "levels/level_definition.h"
#include "math/transform.h"
#include "physics/collision_object.h"
#include "physics/point_constraint.h"
#include "physics/rigid_body.h"
#include "scene/dynamic_scene.h"
#include "sk64/skeletool_animator.h"
#include "sk64/skeletool_armature.h"

#define PLAYER_HEAD_HEIGHT              1.0f
#define PLAYER_CENTER_HEIGHT            0.55f
#define PLAYER_MAX_HEALTH               100.0f

enum PlayerFlags {
    PlayerFlagsGrounded = (1 << 0),
    PlayerHasFirstPortalGun = (1 << 1),
    PlayerHasSecondPortalGun = (1 << 2),
    PlayerCrouched = (1 << 3),
    PlayerIsStepping = (1 << 4),
    PlayerJustJumped = (1 << 5),
    PlayerJustLandedFromFall = (1 << 6),
    PlayerJustSelect = (1 << 7),
    PlayerJustDeniedSelect = (1 << 8),
    PlayerJustShotPortalGun = (1 << 9),
    PlayerInCutscene = (1 << 10),
    PlayerIsInvincible = (1 << 11),
};

enum PlayerDamageType {
    PlayerDamageTypeEnemy,
    PlayerDamageTypeEnvironment,
};

struct Player {
    struct CollisionObject collisionObject;
    struct RigidBody body;
    struct Transform lookTransform;
    struct SKArmature armature;
    struct SKAnimatorBlender animator;
    short grabbingThroughPortal;
    short dynamicId;
    struct PointConstraint grabConstraint;
    struct Quaternion grabRotationBase;
    float pitchVelocity;
    float yawVelocity;
    enum PlayerFlags flags;
    struct RigidBody* anchoredTo;
    struct Vector3 anchorLastPosition;
    short flyingSoundLoopId;
    float health;
    float healthRegenTimer;
    float stepTimer;
    float shakeTimer;
    short currentFoot; //left=0, right=1
    short passedThroughPortal;
    float jumpImpulse;
};

void playerInit(struct Player* player, struct Location* startLocation, struct Vector3* velocity);
void playerUpdate(struct Player* player);
void playerUpdateFooting(struct Player* player, float maxStandDistance);
void playerApplyCameraTransform(struct Player* player, struct Transform* cameraTransform);

void playerGetMoveBasis(struct Quaternion* rotation, struct Vector3* forward, struct Vector3* right);
void playerPortalGrabTransform(struct Player* player, struct Vector3* point, struct Quaternion* rotation);
void playerInitGrabRotationBase(struct Player* player);

void playerGivePortalGun(struct Player* player, int flags);
void playerSetLocation(struct Player* player, struct Location* location);

void playerDamage(struct Player* player, float amount, enum PlayerDamageType damageType);

int playerIsDead(struct Player* player);
void playerSetGrabbing(struct Player* player, struct CollisionObject* grabbing);
void playerSignalPortalChanged(struct Player* player);
int playerIsGrabbing(struct Player* player);
int playerIsGrabbingObject(struct Player* player, struct CollisionObject* object);
void playerThrowObject(struct Player* player);

void playerToggleJumpImpulse(struct Player* player, float newJumpImpulse);
void playerToggleInvincibility(struct Player* player);
void playerToggleCollisionLayers(struct Player* player, short collisionLayers);

#endif