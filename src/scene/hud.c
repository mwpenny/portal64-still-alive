#include "hud.h"

#include "font/dejavu_sans.h"
#include "font/font.h"
#include "graphics/graphics.h"
#include "levels/levels.h"
#include "menu/controls.h"
#include "savefile/savefile.h"
#include "scene.h"
#include "strings/translations.h"
#include "system/screen.h"
#include "util/frame_time.h"
#include "util/memory.h"

#include "codegen/assets/materials/hud.h"
#include "codegen/assets/materials/ui.h"

#define PROMPT_FADE_TIME            2.0f
#define SUBTITLE_SLOW_FADE_TIME     0.75f
#define SUBTITLE_FAST_FADE_TIME     0.25f
#define CAPTION_EXPIRE_TIME         1.5f

#define HUD_OUTER_WIDTH             24
#define HUD_OUTER_HEIGHT            32

#define HUD_OUTER_OFFSET_X          3
#define HUD_OUTER_OFFSET_Y          5
#define HUD_UPPER_X                 ((SCREEN_WD - HUD_OUTER_WIDTH  - (HUD_OUTER_OFFSET_X << 1)) << 1)
#define HUD_UPPER_Y                 ((SCREEN_HT - HUD_OUTER_HEIGHT - (HUD_OUTER_OFFSET_Y << 1)) << 1)
#define HUD_LOWER_X                 ((SCREEN_WD - HUD_OUTER_WIDTH  + (HUD_OUTER_OFFSET_X << 1)) << 1)
#define HUD_LOWER_Y                 ((SCREEN_HT - HUD_OUTER_HEIGHT + (HUD_OUTER_OFFSET_Y << 1)) << 1)

#define RETICLE_XMIN                ((SCREEN_WD - (8 << 1)) << 1)
#define RETICLE_YMIN                ((SCREEN_HT - (8 << 1)) << 1)
#define RETICLE_WIDTH               16
#define RETICLE_HEIGHT              16

#define SUBTITLE_MARGIN_X           17
#define SUBTITLE_MARGIN_Y           11
#define SUBTITLE_PADDING            6

static struct Coloru8 sCrosshairOrange = { 255, 128, 0, 255 };
static struct Coloru8 sCrosshairBlue = { 0, 128, 255, 255 };
static struct Coloru8 sSubtitleTextColor = { 255, 140, 155, 255 };

static enum ControllerAction sPromptActions[] = {
    ControllerActionNone,
    ControllerActionOpenPortal1,
    ControllerActionOpenPortal0,
    ControllerActionUseItem,
    ControllerActionUseItem,
    ControllerActionUseItem,
    ControllerActionDuck,
    ControllerActionMove,
    ControllerActionJump,
};

static enum StringId sPromptText[] = {
    StringIdNone,
    HINT_GET_PORTAL_2,
    HINT_GET_PORTAL_1,
    HINT_USE_ITEMS,
    HINT_DROP_ITEMS,
    HINT_USE_SWITCHES,
    HINT_DUCK,
    HINT_MOVE,
    HINT_JUMP,
};

void hudInit(struct Hud* hud) {
    hud->promptType = CutscenePromptTypeNone;
    hud->subtitleId = StringIdNone;
    hud->queuedSubtitleId = StringIdNone;
    hud->subtitleType = SubtitleTypeNone;
    hud->queuedSubtitleType = SubtitleTypeNone;
    hud->promptOpacity = 0.0f;
    hud->subtitleOpacity = 0.0f;
    hud->backgroundOpacity = 0.0f;
    hud->subtitleFadeTime = SUBTITLE_SLOW_FADE_TIME;
    hud->flags = 0;
    hud->resolvedPrompts = 0;
    hud->lastPortalIndexShot = -1;
    hud->overlayTimer = 0.0f;
}

void hudUpdate(struct Hud* hud) {
    if (hud->overlayTimer > 0.0f) {
        hud->overlayTimer -= FIXED_DELTA_TIME;

        if (hud->overlayTimer < 0.0f) {
            hud->overlayTimer = 0.0f;
        }
    }

    if (hud->subtitleExpireTimer > 0.0f) {
        hud->subtitleExpireTimer -= FIXED_DELTA_TIME;

        if (hud->subtitleExpireTimer < 0.0f) {
            hud->subtitleExpireTimer = 0.0f;
        }
    }

    float targetPromptOpacity = (hud->flags & HudFlagsShowingPrompt) ? 1.0 : 0.0f;
    float targetSubtitleOpacity = ((hud->flags & HudFlagsShowingSubtitle) && (!(hud->flags & HudFlagsSubtitleQueued) || (hud->subtitleExpireTimer > 0.0f))) ? 0.85: 0.0f;
    float targetBackgroundOpacity = (hud->flags & HudFlagsShowingSubtitle && (!(hud->flags & HudFlagsSubtitleQueued) || (hud->subtitleExpireTimer > 0.0f))) ? 0.45: 0.0f;

    if (targetPromptOpacity != hud->promptOpacity) {
        hud->promptOpacity = mathfMoveTowards(hud->promptOpacity, targetPromptOpacity, FIXED_DELTA_TIME / PROMPT_FADE_TIME);
    }

    if (targetSubtitleOpacity != hud->subtitleOpacity) {
        hud->subtitleOpacity = mathfMoveTowards(hud->subtitleOpacity, targetSubtitleOpacity, FIXED_DELTA_TIME / hud->subtitleFadeTime);
    }

    if ((hud->subtitleOpacity <= 0.0f) && (hud->flags & HudFlagsSubtitleQueued)){
        if (!((hud->subtitleType == SubtitleTypeCaption) && (hud->queuedSubtitleType == SubtitleTypeCaption) && (hud->subtitleExpireTimer > 0.0))){
            hud->flags &= ~HudFlagsSubtitleQueued;
            hud->flags |= HudFlagsShowingSubtitle;
            hud->subtitleId = hud->queuedSubtitleId;
            hud->subtitleType = hud->queuedSubtitleType;
            hud->queuedSubtitleId = StringIdNone;
            hud->queuedSubtitleType = SubtitleTypeNone;
            if (hud->subtitleType == SubtitleTypeCaption){
                hud->subtitleExpireTimer = CAPTION_EXPIRE_TIME;
            }
        }
    } else if (hud->subtitleOpacity <= 0.0f && !(hud->flags & HudFlagsSubtitleQueued)) {
        // Allow queuing same subtitle again
        hud->subtitleId = StringIdNone;
    }

    if (hud->subtitleExpireTimer <= 0.0f && hud->subtitleType == SubtitleTypeCaption){
        hudResolveSubtitle(&gScene.hud);
    }

    if (targetBackgroundOpacity != hud->backgroundOpacity) {
        hud->backgroundOpacity = mathfMoveTowards(hud->backgroundOpacity, targetBackgroundOpacity, FIXED_DELTA_TIME / hud->subtitleFadeTime);
    }

    if (targetPromptOpacity && (hud->resolvedPrompts & (1 << hud->promptType)) != 0) {
        hudShowActionPrompt(hud, CutscenePromptTypeNone);
    }

}

void hudUpdatePortalIndicators(struct Hud* hud, struct Ray* raycastRay,  struct Vector3* playerUp) { 
    hud->flags &= ~(HudFlagsLookedPortalable0 | HudFlagsLookedPortalable1);

    if (gScene.player.flags & PlayerHasFirstPortalGun){
        if (sceneFirePortal(&gScene, raycastRay, playerUp, 0, gScene.player.body.currentRoom, 1, 1)) {
            hud->flags |= HudFlagsLookedPortalable0;
        }
        if (sceneFirePortal(&gScene, raycastRay, playerUp, 1, gScene.player.body.currentRoom, 1, 1)) {
            hud->flags |= HudFlagsLookedPortalable1;
        }
    }
}

void hudPortalFired(struct Hud* hud, int index) {
    hud->lastPortalIndexShot = index;

    if (index == 0) {
        hudResolvePrompt(hud, CutscenePromptTypePortal0);
    }
 
    if (index == 1) {
        hudResolvePrompt(hud, CutscenePromptTypePortal1);
    }
}

void hudShowActionPrompt(struct Hud* hud, enum CutscenePromptType promptType) {
    if (promptType == CutscenePromptTypeNone) {
        hud->flags &= ~HudFlagsShowingPrompt;
        return;
    }

    hud->flags |= HudFlagsShowingPrompt;
    hud->promptType = promptType;
}

void hudShowSubtitle(struct Hud* hud, enum StringId subtitleId, enum SubtitleType subtitleType) {
    if (!(gSaveData.video.flags & (VideoSaveFlagsSubtitlesEnabled | VideoSaveFlagsCaptionsEnabled))) {
        return;
    }
    if (subtitleId == hud->subtitleId) {
        return;
    }

    if (subtitleType == SubtitleTypeNone) {
        hud->flags &= ~HudFlagsShowingSubtitle;
        hud->flags &= ~HudFlagsSubtitleQueued;
        hud->queuedSubtitleType = SubtitleTypeNone;
        hud->queuedSubtitleId = StringIdNone;
        hud->subtitleFadeTime = SUBTITLE_SLOW_FADE_TIME;
        return;
    }
    else if (subtitleType == SubtitleTypeCaption) {
        if (!(gSaveData.video.flags & VideoSaveFlagsCaptionsEnabled)){
            return;
        }
        if ((hud->flags & HudFlagsShowingSubtitle) && ((hud->subtitleType > subtitleType) || (hud->queuedSubtitleType > subtitleType))){
            return; // dont push off screen a higher importance subtitle
        }
        else if ((hud->flags & HudFlagsShowingSubtitle) && (hud->subtitleType <= subtitleType)){
            hud->flags |= HudFlagsSubtitleQueued;
            hud->queuedSubtitleId = subtitleId;
            hud->queuedSubtitleType = subtitleType;
            hud->subtitleFadeTime = SUBTITLE_FAST_FADE_TIME;
        }
        else{
            hud->flags |= HudFlagsShowingSubtitle;
            hud->subtitleId = subtitleId;
            hud->subtitleType = subtitleType;
            hud->queuedSubtitleType = SubtitleTypeNone;
            hud->queuedSubtitleId = StringIdNone;
            hud->subtitleFadeTime = SUBTITLE_SLOW_FADE_TIME;
            hud->subtitleExpireTimer = CAPTION_EXPIRE_TIME;
        }
        return;
    }
    else if (subtitleType == SubtitleTypeCloseCaption) {
        if (hud->flags & HudFlagsShowingSubtitle){
            hud->flags |= HudFlagsSubtitleQueued;
            hud->queuedSubtitleId = subtitleId;
            hud->queuedSubtitleType = subtitleType;
            hud->subtitleFadeTime = SUBTITLE_FAST_FADE_TIME;
        }
        else{
            hud->flags |= HudFlagsShowingSubtitle;
            hud->flags &= ~HudFlagsSubtitleQueued;
            hud->subtitleId = subtitleId;
            hud->subtitleType = subtitleType;
            hud->queuedSubtitleType = SubtitleTypeNone;
            hud->queuedSubtitleId = StringIdNone;
            hud->subtitleFadeTime = SUBTITLE_SLOW_FADE_TIME;
        }
        return;
    }
}

void hudResolvePrompt(struct Hud* hud, enum CutscenePromptType promptType) {
    hud->resolvedPrompts |= (1 << promptType);
}

void hudResolveSubtitle(struct Hud* hud) {

    hud->flags &= ~HudFlagsShowingSubtitle;
    hud->subtitleFadeTime = SUBTITLE_SLOW_FADE_TIME;
}

void hudShowColoredOverlay(struct Hud* hud, struct Coloru8* color, float duration, float fadeStartTime) {
    hud->overlayColor = *color;
    hud->overlayTimer = duration;
    hud->overlayFadeStartTime = fadeStartTime;
}

int hudOverlayVisible(struct Hud* hud, struct Player* player) {
    return hud->overlayTimer > 0.0f || player->health < PLAYER_MAX_HEALTH;
}

static void hudRenderOverlay(struct Hud* hud, struct Player* player, struct RenderState* renderState) {
    if (player->health < PLAYER_MAX_HEALTH) {
        float alpha = 1.0f - (player->health * (1.0f / PLAYER_MAX_HEALTH));

        gSPDisplayList(renderState->dl++, hud_overlay);
        gDPSetPrimColor(renderState->dl++, 255, 255, 255, 0, 0, (u8)(128.0f * alpha));
        gDPFillRectangle(renderState->dl++, 0, 0, SCREEN_WD, SCREEN_HT);
        gSPDisplayList(renderState->dl++, hud_overlay_revert);
    }

    if (hud->overlayTimer > 0.0f) {
        float alpha = 0.0f;

        if (hud->overlayTimer > hud->overlayFadeStartTime) {
            alpha = 1.0f;
        } else {
            alpha = hud->overlayTimer * (1.0f / hud->overlayFadeStartTime);
        }

        if (alpha >= 0.0f) {
            gSPDisplayList(renderState->dl++, hud_overlay);
            gDPSetPrimColor(renderState->dl++, 0, 0, hud->overlayColor.r, hud->overlayColor.g, hud->overlayColor.b, (u8)(255.0f * alpha));
            gDPFillRectangle(renderState->dl++, 0, 0, SCREEN_WD, SCREEN_HT);
            gSPDisplayList(renderState->dl++, hud_overlay_revert);
        }
    }
}

static void hudRenderCrosshairs(struct Hud* hud, struct Player* player, struct RenderState* renderState) {
    gSPDisplayList(renderState->dl++, hud_material_list[PORTAL_CROSSHAIRS_INDEX]);

    if (player->flags & PlayerHasFirstPortalGun) {
        int leftTilePos = 0;
        int rightTilePos = HUD_OUTER_WIDTH;
        int indicatorScreenPos = -1;

        struct Coloru8* leftColor;
        struct Coloru8* rightColor;
        struct Coloru8* indicatorColor;

        if (playerIsGrabbing(player)) {
            leftColor = &gColorWhite;
            rightColor = &gColorWhite;
        } else if (!(player->flags & PlayerHasSecondPortalGun)) {
            // Unupgraded portal gun
            leftColor = &sCrosshairBlue;
            rightColor = &sCrosshairBlue;

            if (hud->flags & HudFlagsLookedPortalable1) {
                leftTilePos = HUD_OUTER_WIDTH * 2;
                rightTilePos = HUD_OUTER_WIDTH * 3;
            }
        } else {
            // Fully upgraded portal gun
            leftColor = &sCrosshairOrange;
            rightColor = &sCrosshairBlue;

            if (hud->flags & HudFlagsLookedPortalable0) {
                leftTilePos = HUD_OUTER_WIDTH * 2;
            }
            if (hud->flags & HudFlagsLookedPortalable1) {
                rightTilePos = HUD_OUTER_WIDTH * 3;
            }

            if (hud->lastPortalIndexShot == 0) {
                indicatorScreenPos = HUD_LOWER_X - 68;
                indicatorColor = leftColor;
            } else if (hud->lastPortalIndexShot == 1) {
                indicatorScreenPos = HUD_UPPER_X + 100;
                indicatorColor = rightColor;
            }
        }

        gDPSetPrimColor(renderState->dl++, 255, 255, leftColor->r, leftColor->g, leftColor->b, leftColor->a);
        gSPTextureRectangle(renderState->dl++,
            HUD_UPPER_X, HUD_UPPER_Y,
            HUD_UPPER_X + (HUD_OUTER_WIDTH << 2), HUD_UPPER_Y + (HUD_OUTER_HEIGHT << 2),
            G_TX_RENDERTILE, leftTilePos << 5, 0 << 5, 1 << 10, 1 << 10);
        gDPSetPrimColor(renderState->dl++, 255, 255, rightColor->r, rightColor->g, rightColor->b, rightColor->a);
        gSPTextureRectangle(renderState->dl++,
            HUD_LOWER_X, HUD_LOWER_Y,
            HUD_LOWER_X + (HUD_OUTER_WIDTH << 2), HUD_LOWER_Y + (HUD_OUTER_HEIGHT << 2),
            G_TX_RENDERTILE, rightTilePos << 5, 0 << 5, 1 << 10, 1 << 10);

        if (indicatorScreenPos != -1) {
            gDPSetPrimColor(renderState->dl++, 255, 255, indicatorColor->r, indicatorColor->g, indicatorColor->b, indicatorColor->a);
            gSPTextureRectangle(renderState->dl++,
                indicatorScreenPos, HUD_LOWER_Y,
                indicatorScreenPos + (HUD_OUTER_WIDTH << 2), HUD_LOWER_Y + (HUD_OUTER_HEIGHT << 2),
                G_TX_RENDERTILE, (HUD_OUTER_WIDTH * 4) << 5, 0 << 5, 1 << 10, 1 << 10);
        }
    }

    if ((!playerIsDead(player) && !(player->flags & PlayerInCutscene)) &&
        (gCurrentLevelIndex > 0 || hud->overlayTimer <= 0.0f)) {
        // Center reticle is drawn over top everything
        gSPDisplayList(renderState->dl++, hud_material_list[CENTER_RETICLE_INDEX]);
        gDPSetPrimColor(renderState->dl++, 255, 255, 210, 210, 210, 255);
        gSPTextureRectangle(renderState->dl++,
            RETICLE_XMIN, RETICLE_YMIN,
            RETICLE_XMIN + (RETICLE_WIDTH << 2), RETICLE_YMIN + (RETICLE_HEIGHT << 2),
            G_TX_RENDERTILE, 0 << 5, 0 << 5, 1 << 10, 1 << 10);
    }
}

static void hudRenderSubtitle(char* message, float textOpacity, float backgroundOpacity, struct RenderState* renderState, enum SubtitleType subtitleType) {
    if (message == NULL || *message == '\0') {
        return;
    }

    struct FontRenderer* fontRender = stackMalloc(sizeof(struct FontRenderer));
    fontRendererLayout(fontRender, &gDejaVuSansFont, message, SCREEN_WD - (SUBTITLE_MARGIN_X + SUBTITLE_PADDING) * 2);

    int textPositionX = (SUBTITLE_MARGIN_X + SUBTITLE_PADDING);
    int textPositionY = (SCREEN_HT - SUBTITLE_MARGIN_Y - SUBTITLE_PADDING) - fontRender->height;

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);
    gDPSetEnvColor(renderState->dl++, 0, 0, 0, (u8)(255.0f * backgroundOpacity));
    gDPFillRectangle(
        renderState->dl++,
        textPositionX - SUBTITLE_PADDING,
        textPositionY - SUBTITLE_PADDING,
        SCREEN_WD - SUBTITLE_MARGIN_X,
        SCREEN_HT - SUBTITLE_MARGIN_Y
    );
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);

    struct Coloru8 textColor;

    if (subtitleType == SubtitleTypeCloseCaption) {
        textColor.r = sSubtitleTextColor.r;
        textColor.g = sSubtitleTextColor.g;
        textColor.b = sSubtitleTextColor.b;
    } else if (subtitleType == SubtitleTypeCaption) {
        textColor = gColorWhite;
    }

    textColor.a = (u8)(255.0f * textOpacity);

    renderState->dl = fontRendererBuildGfx(fontRender, gDejaVuSansImages, textPositionX, textPositionY, &textColor, renderState->dl);

    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_0_INDEX]);

    stackMallocFree(fontRender);
}

void hudRender(struct Hud* hud, struct Player* player, struct RenderState* renderState) {
    hudRenderOverlay(hud, player, renderState);
    hudRenderCrosshairs(hud, player, renderState);

    if (hud->subtitleOpacity > 0.0f && (gSaveData.video.flags & (VideoSaveFlagsSubtitlesEnabled | VideoSaveFlagsCaptionsEnabled)) && hud->subtitleId != StringIdNone) {
        hudRenderSubtitle(translationsGet(hud->subtitleId), hud->subtitleOpacity, hud->backgroundOpacity, renderState, hud->subtitleType);
    }

    if (hud->promptOpacity > 0.0f && hud->promptType != CutscenePromptTypeNone) {
        controlsRenderPrompt(sPromptActions[hud->promptType], translationsGet(sPromptText[hud->promptType]), hud->promptOpacity, renderState);
    }
}
