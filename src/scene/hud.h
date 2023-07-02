#ifndef __SCENE_HUD_H__
#define __SCENE_HUD_H__

#include "../graphics/renderstate.h"
#include "../player/player.h"

#define INTRO_BLACK_TIME 3.0f
#define INTRO_FADE_TIME  1.0f
#define INTRO_TOTAL_TIME  (INTRO_BLACK_TIME + INTRO_FADE_TIME)

void hudRender(struct RenderState* renderState, struct Player* player, int last_portal_idx_shot, int looked_wall_portalable_0, int looked_wall_portalable_1, float introAnimationTime);

#endif