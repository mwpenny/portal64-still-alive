#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "../math/transform.h"
#include "../graphics/renderstate.h"
#include "../physics/rigid_body.h"
#include "../physics/collision_object.h"
#include "../levels/level_definition.h"
#include "../scene/dynamic_scene.h"
#include "../sk64/skelatool_animator.h"
#include "../sk64/skelatool_armature.h"
#include "../physics/point_constraint.h"

#define PLAYER_GRABBING_THROUGH_NOTHING -1

#define PLAYER_HEAD_HEIGHT              1.0f

enum PlayerFlags {
    PlayerFlagsGrounded = (1 << 0),
    PlayerHasFirstPortalGun = (1 << 1),
    PlayerHasSecondPortalGun = (1 << 2),
    PlayerIsDead = (1 << 3),
    PlayerIsUnderwater = (1 << 4),
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
    float pitchVelocity;
    float yawVelocity;
    enum PlayerFlags flags;
    struct RigidBody* anchoredTo;
    struct Vector3 relativeAnchor;
    struct Vector3 lastAnchorPoint;
    float drownTimer;
};

void playerInit(struct Player* player, struct Location* startLocation, struct Vector3* velocity);
void playerUpdate(struct Player* player, struct Transform* cameraTransform);

void playerGetMoveBasis(struct Transform* transform, struct Vector3* forward, struct Vector3* right);

void playerGivePortalGun(struct Player* player, int flags);

void playerKill(struct Player* player, int isUnderwater);

int playerIsDead(struct Player* player);
void playerSetGrabbing(struct Player* player, struct CollisionObject* grabbing);

#endif