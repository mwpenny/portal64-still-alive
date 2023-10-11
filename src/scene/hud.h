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
    HudFlagsShowingSubtitle= (1 << 3),
    HudFlagsSubtitleQueued= (1 << 4),
};

struct Hud {
    int chosenLanguage;
    enum CutscenePromptType promptType;
    enum SubtitleKey subtitleType;
    enum SubtitleKey queuedSubtitleType;
    float promptOpacity;
    float subtitleOpacity;
    float backgroundOpacity;

    float subtitleFadeTime;
    float fadeInTimer;

    u16 flags;
    u16 resolvedPrompts;

    u8 lastPortalIndexShot;
};

void hudInit(struct Hud* hud);

void hudUpdate(struct Hud* hud);
void hudUpdatePortalIndicators(struct Hud* hud, struct Ray* raycastRay,  struct Vector3* playerUp);

void hudPortalFired(struct Hud* hud, int index);
void hudShowActionPrompt(struct Hud* hud, enum CutscenePromptType promptType);
void hudResolvePrompt(struct Hud* hud, enum CutscenePromptType promptType);
void hudShowSubtitle(struct Hud* hud, enum SubtitleKey subtitleType);
void hudResolveSubtitle(struct Hud* hud);

void hudRender(struct Hud* hud, struct Player* player, struct RenderState* renderState);

#endif