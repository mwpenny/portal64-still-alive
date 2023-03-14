#ifndef __SCENE_HUD_H__
#define __SCENE_HUD_H__

#include "../graphics/renderstate.h"
#include "../player/player.h"

void hudRender(struct RenderState* renderState, struct Player* player, int last_portal_idx_shot, int looked_wall_portalable_0, int looked_wall_portalable_1);

#endif