#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "../math/transform.h"
#include "../graphics/renderstate.h"
#include "../physics/rigid_body.h"

struct Player {
    struct Transform transform;
    struct RigidBody* grabbing;
    struct Vector3 velocity;
    float pitch;
    float pitchVelocity;
    float yaw;
    float yawVelocity;
};

void playerInit(struct Player* player);
void playerUpdate(struct Player* player, struct Transform* cameraTransform);

void playerRender(struct Player* player, struct RenderState* renderState);

#endif