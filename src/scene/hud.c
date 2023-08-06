#include "hud.h"

#include "../../build/assets/materials/hud.h"
#include "../graphics/graphics.h"

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

void hudRender(struct RenderState* renderState, struct Player* player, int last_portal_idx_shot, int looked_wall_portalable_0, int looked_wall_portalable_1, float introAnimationTime) {
    if (player->flags & PlayerIsDead) {
        gSPDisplayList(renderState->dl++, hud_death_overlay);
        gDPFillRectangle(renderState->dl++, 0, 0, SCREEN_WD, SCREEN_HT);
        gSPDisplayList(renderState->dl++, hud_death_overlay_revert);
    }

    if (introAnimationTime > 0.0f) {
        float alpha = 0.0f;

        if (introAnimationTime > INTRO_FADE_TIME) {
            alpha = 1.0f;
        } else {
            alpha = introAnimationTime * (1.0f / INTRO_FADE_TIME);
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
            if (looked_wall_portalable_1){
                position_of_right_asset = (HUD_OUTER_WIDTH*3);
            }
            gSPTextureRectangle(renderState->dl++, 
                HUD_LOWER_X, HUD_LOWER_Y,
                HUD_LOWER_X + (HUD_OUTER_WIDTH << 2), HUD_LOWER_Y + (HUD_OUTER_HEIGHT << 2), 
                G_TX_RENDERTILE, position_of_right_asset << 5, 0 << 5, 1 << 10, 1 << 10);
            
            // if the player has the first gun but not second both left and right are blue
            if (!(player->flags & PlayerHasSecondPortalGun)){
                if (looked_wall_portalable_1){
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
            if (looked_wall_portalable_0){
                position_of_left_asset = (HUD_OUTER_WIDTH*2);
            }
            gSPTextureRectangle(renderState->dl++, 
                HUD_UPPER_X, HUD_UPPER_Y,
                HUD_UPPER_X + (HUD_OUTER_WIDTH << 2), HUD_UPPER_Y + (HUD_OUTER_HEIGHT << 2), 
                G_TX_RENDERTILE, position_of_left_asset << 5, 0 << 5, 1 << 10, 1 << 10);
        }
    }
    

    // both portal guns owned is only time when the last shot portal indicator appears
    if ((player->flags & PlayerHasSecondPortalGun) && (player->flags & PlayerHasFirstPortalGun) && last_portal_idx_shot != -1){
        //orange indicator
        if (last_portal_idx_shot == 0){
            gDPSetPrimColor(renderState->dl++, 255, 255, 255, 128, 0, 255);
            gSPTextureRectangle(renderState->dl++, 
                HUD_LOWER_X - 68, HUD_LOWER_Y,
                HUD_LOWER_X - 68 + (HUD_OUTER_WIDTH << 2), HUD_LOWER_Y + (HUD_OUTER_HEIGHT << 2), 
                G_TX_RENDERTILE, position_of_portal_indicator << 5, 0 << 5, 1 << 10, 1 << 10);
        }
        //blue indicator
        else if (last_portal_idx_shot == 1){
            gDPSetPrimColor(renderState->dl++, 255, 255, 0, 128, 255, 255);
            gSPTextureRectangle(renderState->dl++, 
                HUD_UPPER_X + 100, HUD_LOWER_Y,
                HUD_UPPER_X + 100 + (HUD_OUTER_WIDTH << 2), HUD_LOWER_Y + (HUD_OUTER_HEIGHT << 2), 
                G_TX_RENDERTILE, position_of_portal_indicator << 5, 0 << 5, 1 << 10, 1 << 10);
        }
    }

    if (introAnimationTime <= 0.0f) {
        // center reticle is drawn over top everything
        gSPDisplayList(renderState->dl++, hud_material_list[CENTER_RETICLE_INDEX]);
        gDPSetPrimColor(renderState->dl++, 255, 255, 210, 210, 210, 255);
        gSPTextureRectangle(renderState->dl++, 
                RETICLE_XMIN, RETICLE_YMIN,
                RETICLE_XMIN + (RETICLE_WIDTH << 2), RETICLE_YMIN + (RETICLE_HEIGHT << 2), 
                G_TX_RENDERTILE, 0 << 5, 0 << 5, 1 << 10, 1 << 10);
    }
}