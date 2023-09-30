#ifndef __SCENE_HUD_H__
#define __SCENE_HUD_H__

#include "../graphics/renderstate.h"
#include "../player/player.h"
#include "../controls/controller_actions.h"

#define INTRO_BLACK_TIME 3.0f
#define INTRO_FADE_TIME  1.0f
#define INTRO_TOTAL_TIME  (INTRO_BLACK_TIME + INTRO_FADE_TIME)

enum HudFlags {
    HudFlagsLookedPortalable0 = (1 << 0),
    HudFlagsLookedPortalable1 = (1 << 1),
    HudFlagsShowingPrompt = (1 << 2),
};

struct Hud {
    enum ControllerAction promptAction;
    char* promptMessage;
    float promptOpacity;

    float fadeInTimer;

    u16 flags;

    u8 lastPortalIndexShot;
};

void hudInit(struct Hud* hud);

void hudUpdate(struct Hud* hud);
void hudUpdatePortalIndicators(struct Hud* hud, struct Ray* raycastRay,  struct Vector3* playerUp);

void hudPortalFired(struct Hud* hud, int index);

void hudRender(struct Hud* hud, struct Player* player, struct RenderState* renderState);

#endif