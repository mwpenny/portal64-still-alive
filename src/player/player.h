#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "../math/transform.h"
#include "../graphics/renderstate.h"
#include "../physics/rigid_body.h"
#include "../physics/collision_object.h"
#include "../levels/level_definition.h"

#define PLAYER_GRABBING_THROUGH_NOTHING -1

#define PLAYER_HEAD_HEIGHT              1.0f

enum PlayerFlags {
    PlayerFlagsGrounded = (1 << 0),
};

struct Player {
    struct CollisionObject collisionObject;
    struct RigidBody body;
    struct Transform lookTransform;
    short grabbingThroughPortal;
    struct CollisionObject* grabbing;
    float pitchVelocity;
    float yawVelocity;
    enum PlayerFlags flags;
};

void playerInit(struct Player* player, struct Location* startLocation, struct Vector3* velocity);
void playerUpdate(struct Player* player, struct Transform* cameraTransform);

void playerRender(struct Player* player, struct RenderState* renderState);

void playerGetMoveBasis(struct Transform* transform, struct Vector3* forward, struct Vector3* right);

#endif