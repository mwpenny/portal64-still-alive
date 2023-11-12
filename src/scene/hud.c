#include "hud.h"

#include "../../build/assets/materials/hud.h"
#include "../menu/controls.h"
#include "../graphics/graphics.h"
#include "../util/time.h"
#include "../levels/levels.h"
#include "./scene.h"
#include "../savefile/savefile.h"
#include "../menu/translations.h"

#define HUD_CENTER_WIDTH 6
#define HUD_CENTER_HEIGHT 8

#define HUD_CENTER_S 101
#define HUD_CENTER_T 12

#define HUD_OUTER_WIDTH  24
#define HUD_OUTER_HEIGHT 32

#define HUD_OUTER_OFFSET_X 3
#define HUD_OUTER_OFFSET_Y 5

#define HUD_UPPER_X ((SCREEN_WD - HUD_OUTER_WIDTH - (HUD_OUTER_OFFSET_X << 1)) << 1)
#define HUD_UPPER_Y ((SCREEN_HT - HUD_OUTER_HEIGHT - (HUD_OUTER_OFFSET_Y << 1)) << 1)

#define HUD_LOWER_X ((SCREEN_WD - HUD_OUTER_WIDTH + (HUD_OUTER_OFFSET_X << 1)) << 1)
#define HUD_LOWER_Y ((SCREEN_HT - HUD_OUTER_HEIGHT + (HUD_OUTER_OFFSET_Y << 1)) << 1)

#define RETICLE_XMIN ((SCREEN_WD - (8 << 1)) << 1)
#define RETICLE_YMIN ((SCREEN_HT - (8 << 1)) << 1)
#define RETICLE_WIDTH 16
#define RETICLE_HEIGHT 16

#define PROMPT_FADE_TIME               2.0f
#define SUBTITLE_SLOW_FADE_TIME        0.75f
#define SUBTITLE_FAST_FADE_TIME        0.25f
#define CAPTION_EXPIRE_TIME            1.5f



void hudInit(struct Hud* hud) {
    hud->promptType = CutscenePromptTypeNone;
    hud->subtitleKey = SubtitleKeyNone;
    hud->queuedSubtitleKey = SubtitleKeyNone;
    hud->subtitleType = SubtitleTypeNone;
    hud->queuedSubtitleType = SubtitleTypeNone;
    hud->promptOpacity = 0.0f;
    hud->subtitleOpacity = 0.0f;
    hud->backgroundOpacity = 0.0f;
    hud->subtitleFadeTime = SUBTITLE_SLOW_FADE_TIME;
    hud->flags = 0;
    hud->resolvedPrompts = 0;
    hud->lastPortalIndexShot = -1;

    if (gCurrentLevelIndex == 0) {
        hud->fadeInTimer = INTRO_TOTAL_TIME;
    } else {
        hud->fadeInTimer = 0.0f;
    }
}


void hudUpdate(struct Hud* hud) {
    if (hud->fadeInTimer > 0.0f) {
        hud->fadeInTimer -= FIXED_DELTA_TIME;

        if (hud->fadeInTimer < 0.0f) {
            hud->fadeInTimer = 0.0f;
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
            hud->subtitleKey = hud->queuedSubtitleKey;
            hud->subtitleType= hud->queuedSubtitleType;
            hud->queuedSubtitleKey = SubtitleKeyNone;
            hud->queuedSubtitleType = SubtitleTypeNone;
            if (hud->subtitleType == SubtitleTypeCaption){
                hud->subtitleExpireTimer = CAPTION_EXPIRE_TIME;
            }
        }
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

u8 gPromptActions[] = {
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

int gPromptText[] = {
    NULL,
    HINT_GET_PORTAL_2,
    HINT_GET_PORTAL_1,
    HINT_USE_ITEMS,
    HINT_DROP_ITEMS,
    HINT_USE_SWITCHES,
    HINT_DUCK,
    HINT_MOVE,
    HINT_JUMP,
};

void hudShowActionPrompt(struct Hud* hud, enum CutscenePromptType promptType) {
    if (promptType == CutscenePromptTypeNone) {
        hud->flags &= ~HudFlagsShowingPrompt;
        return;
    }

    hud->flags |= HudFlagsShowingPrompt;
    hud->promptType = promptType;
}

void hudShowSubtitle(struct Hud* hud, enum SubtitleKey subtitleKey, enum SubtitleType subtitleType) {
    if (!(gSaveData.controls.flags & ControlSaveSubtitlesEnabled || gSaveData.controls.flags & ControlSaveAllSubtitlesEnabled)){
        return;
    }
    if (subtitleKey == hud->subtitleKey){
        return;
    }

    if (subtitleType == SubtitleTypeNone) {
        hud->flags &= ~HudFlagsShowingSubtitle;
        hud->flags &= ~HudFlagsSubtitleQueued;
        hud->queuedSubtitleType = SubtitleTypeNone;
        hud->queuedSubtitleKey = SubtitleKeyNone;
        hud->subtitleFadeTime = SUBTITLE_SLOW_FADE_TIME;
        return;
    }
    else if (subtitleType == SubtitleTypeCaption) {
        if (!(gSaveData.controls.flags & ControlSaveAllSubtitlesEnabled)){
            return;
        }
        if ((hud->flags & HudFlagsShowingSubtitle) && ((hud->subtitleType > subtitleType) || (hud->queuedSubtitleType > subtitleType))){
            return; // dont push off screen a higher importance subtitle
        }
        else if ((hud->flags & HudFlagsShowingSubtitle) && (hud->subtitleType <= subtitleType)){
            hud->flags |= HudFlagsSubtitleQueued;
            hud->queuedSubtitleKey = subtitleKey;
            hud->queuedSubtitleType = subtitleType;
            hud->subtitleFadeTime = SUBTITLE_FAST_FADE_TIME;
        }
        else{
            hud->flags |= HudFlagsShowingSubtitle;
            hud->subtitleKey = subtitleKey;
            hud->subtitleType = subtitleType;
            hud->queuedSubtitleType = SubtitleTypeNone;
            hud->queuedSubtitleKey = SubtitleKeyNone;
            hud->subtitleFadeTime = SUBTITLE_SLOW_FADE_TIME;
            hud->subtitleExpireTimer = CAPTION_EXPIRE_TIME;
        }
        return;
    }
    else if (subtitleType == SubtitleTypeCloseCaption) {
        if (hud->flags & HudFlagsShowingSubtitle){
            hud->flags |= HudFlagsSubtitleQueued;
            hud->queuedSubtitleKey = subtitleKey;
            hud->queuedSubtitleType = subtitleType;
            hud->subtitleFadeTime = SUBTITLE_FAST_FADE_TIME;
        }
        else{
            hud->flags |= HudFlagsShowingSubtitle;
            hud->flags &= ~HudFlagsSubtitleQueued;
            hud->subtitleKey = subtitleKey;
            hud->subtitleType = subtitleType;
            hud->queuedSubtitleType = SubtitleTypeNone;
            hud->queuedSubtitleKey = SubtitleKeyNone;
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

void hudRender(struct Hud* hud, struct Player* player, struct RenderState* renderState) {
    if (player->flags & PlayerIsDead) {
        gSPDisplayList(renderState->dl++, hud_death_overlay);
        gDPFillRectangle(renderState->dl++, 0, 0, SCREEN_WD, SCREEN_HT);
        gSPDisplayList(renderState->dl++, hud_death_overlay_revert);
    }

    if (hud->fadeInTimer > 0.0f) {
        float alpha = 0.0f;

        if (hud->fadeInTimer > INTRO_FADE_TIME) {
            alpha = 1.0f;
        } else {
            alpha = hud->fadeInTimer * (1.0f / INTRO_FADE_TIME);
        }

        if (alpha >= 0.0f) {
            gSPDisplayList(renderState->dl++, hud_death_overlay);
            gDPSetPrimColor(renderState->dl++, 0, 0, 0, 0, 0, (u8)(255.0f * alpha));
            gDPFillRectangle(renderState->dl++, 0, 0, SCREEN_WD, SCREEN_HT);
            gSPDisplayList(renderState->dl++, hud_death_overlay_revert);
        }
    }

    gSPDisplayList(renderState->dl++, hud_material_list[PORTAL_CROSSHAIRS_INDEX]);

    int position_of_left_asset = 0; // position from original image to clip out left hud asset
    int position_of_right_asset = HUD_OUTER_WIDTH; // position from original image to clip out right hud asset
    int position_of_portal_indicator = HUD_OUTER_WIDTH*4;

    // white hud because player is grabbing
    if ((playerIsGrabbing(player)) && (player->flags & PlayerHasFirstPortalGun) ){
        gDPSetPrimColor(renderState->dl++, 255, 255, 255, 255, 255, 255);
        gSPTextureRectangle(renderState->dl++, 
            HUD_UPPER_X, HUD_UPPER_Y,
            HUD_UPPER_X + (HUD_OUTER_WIDTH << 2), HUD_UPPER_Y + (HUD_OUTER_HEIGHT << 2), 
            G_TX_RENDERTILE, position_of_left_asset << 5, 0 << 5, 1 << 10, 1 << 10);
        gSPTextureRectangle(renderState->dl++, 
                HUD_LOWER_X, HUD_LOWER_Y,
                HUD_LOWER_X + (HUD_OUTER_WIDTH << 2), HUD_LOWER_Y + (HUD_OUTER_HEIGHT << 2), 
                G_TX_RENDERTILE, position_of_right_asset << 5, 0 << 5, 1 << 10, 1 << 10);
    }
    // else blue and orange logic
    else{
        //blue drawing
        if (player->flags & PlayerHasFirstPortalGun){
            gDPSetPrimColor(renderState->dl++, 255, 255, 0, 128, 255, 255);
            if (hud->flags & HudFlagsLookedPortalable1){
                position_of_right_asset = (HUD_OUTER_WIDTH*3);
            }
            gSPTextureRectangle(renderState->dl++, 
                HUD_LOWER_X, HUD_LOWER_Y,
                HUD_LOWER_X + (HUD_OUTER_WIDTH << 2), HUD_LOWER_Y + (HUD_OUTER_HEIGHT << 2), 
                G_TX_RENDERTILE, position_of_right_asset << 5, 0 << 5, 1 << 10, 1 << 10);
            
            // if the player has the first gun but not second both left and right are blue
            if (!(player->flags & PlayerHasSecondPortalGun)){
                if (hud->flags & HudFlagsLookedPortalable1){
                    position_of_left_asset = (HUD_OUTER_WIDTH*2);
                }   
                gSPTextureRectangle(renderState->dl++, 
                    HUD_UPPER_X, HUD_UPPER_Y,
                    HUD_UPPER_X + (HUD_OUTER_WIDTH << 2), HUD_UPPER_Y + (HUD_OUTER_HEIGHT << 2),
                    G_TX_RENDERTILE, position_of_left_asset << 5, 0 << 5, 1 << 10, 1 << 10);
            }
            
        }
        //orange drawing
        if (player->flags & PlayerHasSecondPortalGun){
            gDPSetPrimColor(renderState->dl++, 255, 255, 255, 128, 0, 255);
            if (hud->flags & HudFlagsLookedPortalable0){
                position_of_left_asset = (HUD_OUTER_WIDTH*2);
            }
            gSPTextureRectangle(renderState->dl++, 
                HUD_UPPER_X, HUD_UPPER_Y,
                HUD_UPPER_X + (HUD_OUTER_WIDTH << 2), HUD_UPPER_Y + (HUD_OUTER_HEIGHT << 2), 
                G_TX_RENDERTILE, position_of_left_asset << 5, 0 << 5, 1 << 10, 1 << 10);
        }
    }
    

    // both portal guns owned is only time when the last shot portal indicator appears
    if ((player->flags & PlayerHasSecondPortalGun) && (player->flags & PlayerHasFirstPortalGun) && hud->lastPortalIndexShot != -1){
        //orange indicator
        if (hud->lastPortalIndexShot == 0){
            gDPSetPrimColor(renderState->dl++, 255, 255, 255, 128, 0, 255);
            gSPTextureRectangle(renderState->dl++, 
                HUD_LOWER_X - 68, HUD_LOWER_Y,
                HUD_LOWER_X - 68 + (HUD_OUTER_WIDTH << 2), HUD_LOWER_Y + (HUD_OUTER_HEIGHT << 2), 
                G_TX_RENDERTILE, position_of_portal_indicator << 5, 0 << 5, 1 << 10, 1 << 10);
        }
        //blue indicator
        else if (hud->lastPortalIndexShot == 1){
            gDPSetPrimColor(renderState->dl++, 255, 255, 0, 128, 255, 255);
            gSPTextureRectangle(renderState->dl++, 
                HUD_UPPER_X + 100, HUD_LOWER_Y,
                HUD_UPPER_X + 100 + (HUD_OUTER_WIDTH << 2), HUD_LOWER_Y + (HUD_OUTER_HEIGHT << 2), 
                G_TX_RENDERTILE, position_of_portal_indicator << 5, 0 << 5, 1 << 10, 1 << 10);
        }
    }

    if (hud->fadeInTimer <= 0.0f) {
        // center reticle is drawn over top everything
        gSPDisplayList(renderState->dl++, hud_material_list[CENTER_RETICLE_INDEX]);
        gDPSetPrimColor(renderState->dl++, 255, 255, 210, 210, 210, 255);
        gSPTextureRectangle(renderState->dl++, 
                RETICLE_XMIN, RETICLE_YMIN,
                RETICLE_XMIN + (RETICLE_WIDTH << 2), RETICLE_YMIN + (RETICLE_HEIGHT << 2), 
                G_TX_RENDERTILE, 0 << 5, 0 << 5, 1 << 10, 1 << 10);
    }

    if (hud->promptOpacity > 0.0f && hud->promptType != CutscenePromptTypeNone) {
        controlsRenderPrompt(gPromptActions[hud->promptType], translationsGet(gPromptText[hud->promptType]), hud->promptOpacity, renderState);
    }

    if (hud->subtitleOpacity > 0.0f && (gSaveData.controls.flags & ControlSaveSubtitlesEnabled || gSaveData.controls.flags & ControlSaveAllSubtitlesEnabled) && hud->subtitleKey != SubtitleKeyNone) {
        controlsRenderSubtitle(translationsGet(hud->subtitleKey), hud->subtitleOpacity, hud->backgroundOpacity, renderState, hud->subtitleType);
    }
}
