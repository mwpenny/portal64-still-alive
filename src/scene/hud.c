#include "hud.h"

#include "../../build/assets/materials/hud.h"
#include "../graphics/graphics.h"

#include "../player/player.h"

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

void hudRender(struct RenderState* renderState, int playerFlags, int last_portal_idx_shot, int looked_wall_portalable) {
    if (playerFlags & PlayerIsDead) {
        gSPDisplayList(renderState->dl++, hud_death_overlay);
        gDPFillRectangle(renderState->dl++, 0, 0, SCREEN_WD, SCREEN_HT);
        gSPDisplayList(renderState->dl++, hud_death_overlay_revert);
    }

    gSPDisplayList(renderState->dl++, hud_material_list[PORTAL_CROSSHAIRS_INDEX]);

    int position_of_left_asset = 0; // position from original image to clip out left hud asset
    int position_of_right_asset = HUD_OUTER_WIDTH; // position from original image to clip out right hud asset
    int position_of_portal_indicator = HUD_OUTER_WIDTH*4;

    //blue drawing
    if (playerFlags & PlayerHasFirstPortalGun){
        gDPSetPrimColor(renderState->dl++, 255, 255, 0, 128, 255, 255);
        if (looked_wall_portalable){
            position_of_left_asset = (HUD_OUTER_WIDTH*2);
        }
        gSPTextureRectangle(renderState->dl++, 
            HUD_UPPER_X, HUD_UPPER_Y,
            HUD_UPPER_X + (HUD_OUTER_WIDTH << 2), HUD_UPPER_Y + (HUD_OUTER_HEIGHT << 2), 
            G_TX_RENDERTILE, position_of_left_asset << 5, 0 << 5, 1 << 10, 1 << 10);
        
        // if the player has the first gun but not second both left and right are blue
        if (!(playerFlags & PlayerHasSecondPortalGun)){
            if (looked_wall_portalable){
                position_of_right_asset = (HUD_OUTER_WIDTH*3);
            }   
            gSPTextureRectangle(renderState->dl++, 
                HUD_LOWER_X, HUD_LOWER_Y,
                HUD_LOWER_X + (HUD_OUTER_WIDTH << 2), HUD_LOWER_Y + (HUD_OUTER_HEIGHT << 2), 
                G_TX_RENDERTILE, position_of_right_asset << 5, 0 << 5, 1 << 10, 1 << 10);
        }
        
    }

    //orange drawing
    if (playerFlags & PlayerHasSecondPortalGun){
        gDPSetPrimColor(renderState->dl++, 255, 255, 255, 128, 0, 255);
        if (looked_wall_portalable){
            position_of_right_asset = (HUD_OUTER_WIDTH*3);
        }
        gSPTextureRectangle(renderState->dl++, 
            HUD_LOWER_X, HUD_LOWER_Y,
            HUD_LOWER_X + (HUD_OUTER_WIDTH << 2), HUD_LOWER_Y + (HUD_OUTER_HEIGHT << 2), 
            G_TX_RENDERTILE, position_of_right_asset << 5, 0 << 5, 1 << 10, 1 << 10);
    }

    // both portal guns owned is only time when the last shot portal indicator appears
    if ((playerFlags & PlayerHasSecondPortalGun) && (playerFlags & PlayerHasFirstPortalGun) && last_portal_idx_shot != -1){
        if (last_portal_idx_shot == 0){
            gDPSetPrimColor(renderState->dl++, 255, 255, 255, 128, 0, 255);
            gSPTextureRectangle(renderState->dl++, 
                HUD_UPPER_X + 100, HUD_LOWER_Y,
                HUD_UPPER_X + 100 + (HUD_OUTER_WIDTH << 2), HUD_LOWER_Y + (HUD_OUTER_HEIGHT << 2) , 
                G_TX_RENDERTILE, position_of_portal_indicator << 5, 0 << 5, 1 << 10, 1 << 10);
        }
        else if (last_portal_idx_shot == 1){
            gDPSetPrimColor(renderState->dl++, 255, 255, 0, 128, 255, 255);
            gSPTextureRectangle(renderState->dl++, 
                HUD_LOWER_X - 68, HUD_LOWER_Y,
                HUD_LOWER_X - 68 + (HUD_OUTER_WIDTH << 2), HUD_LOWER_Y + (HUD_OUTER_HEIGHT << 2) , 
                G_TX_RENDERTILE, position_of_portal_indicator << 5, 0 << 5, 1 << 10, 1 << 10);
        }
    }
}