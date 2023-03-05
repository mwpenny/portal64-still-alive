#ifndef __SCENE_HUD_H__
#define __SCENE_HUD_H__

#include "../graphics/renderstate.h"

void hudRender(struct RenderState* renderState, int playerFlags, int last_portal_idx_shot, int looked_wall_portalable);

#endif