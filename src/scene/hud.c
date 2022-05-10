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

void hudRender(struct RenderState* renderState) {
    gSPDisplayList(renderState->dl++, hud_material_list[PORTAL_CROSSHAIRS_INDEX]);

    gSPTextureRectangle(renderState->dl++, 
        (SCREEN_WD - HUD_CENTER_WIDTH) << 1, (SCREEN_HT - HUD_CENTER_HEIGHT) << 1, 
        (SCREEN_WD + HUD_CENTER_WIDTH) << 1, (SCREEN_HT + HUD_CENTER_HEIGHT) << 1, 
        G_TX_RENDERTILE, HUD_CENTER_S << 5, HUD_CENTER_T << 5, 1 << 10, 1 << 10);

    gDPSetPrimColor(renderState->dl++, 255, 255, 255, 128, 0, 255);

    gSPTextureRectangle(renderState->dl++, 
        HUD_UPPER_X, HUD_UPPER_Y,
        HUD_UPPER_X + (HUD_OUTER_WIDTH << 2), HUD_UPPER_Y + (HUD_OUTER_HEIGHT << 2), 
        G_TX_RENDERTILE, 0 << 5, 0 << 5, 1 << 10, 1 << 10);

    gDPSetPrimColor(renderState->dl++, 255, 255, 0, 128, 255, 255);

    gSPTextureRectangle(renderState->dl++, 
        HUD_LOWER_X, HUD_LOWER_Y,
        HUD_LOWER_X + (HUD_OUTER_WIDTH << 2), HUD_LOWER_Y + (HUD_OUTER_HEIGHT << 2), 
        G_TX_RENDERTILE, HUD_OUTER_WIDTH << 5, 0 << 5, 1 << 10, 1 << 10);
}