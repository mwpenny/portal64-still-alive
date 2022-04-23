#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "../math/transform.h"
#include "../graphics/renderstate.h"
#include "../physics/rigid_body.h"
#include "../physics/collision_object.h"

#define PLAYER_GRABBING_THROUGH_NOTHING -1

#define PLAYER_HEAD_HEIGHT              1.2f

enum PlayerFlags {
    PlayerFlagsGrounded = (1 << 0),
};

struct Player {
    struct CollisionObject collisionObject;
    struct RigidBody body;
    short grabbingThroughPortal;
    struct RigidBody* grabbing;
    float pitchVelocity;
    float yawVelocity;
    enum PlayerFlags flags;
};

void playerInit(struct Player* player);
void playerUpdate(struct Player* player, struct Transform* cameraTransform);

void playerRender(struct Player* player, struct RenderState* renderState);

#endif