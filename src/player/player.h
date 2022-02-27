#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "../math/transform.h"
#include "../graphics/renderstate.h"

struct Player {
    struct Transform transform;
};

void playerInit(struct Player* player);
void playerUpdate(struct Player* player, struct Transform* cameraTransform);

void playerRender(struct Player* player, struct RenderState* renderState);

#endif