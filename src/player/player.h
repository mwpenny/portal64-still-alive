#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "../math/transform.h"
#include "../graphics/renderstate.h"
#include "../physics/rigid_body.h"
#include "../physics/collision_object.h"
#include "../levels/level_definition.h"

#define PLAYER_GRABBING_THROUGH_NOTHING -1

#define PLAYER_HEAD_HEIGHT              0.8f

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

void playerInit(struct Player* player, struct Location* startLocation);
void playerUpdate(struct Player* player, struct Transform* cameraTransform);

void playerRender(struct Player* player, struct RenderState* renderState);

#endif