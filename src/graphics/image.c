#include "image.h"

#include "graphics.h"

#define MAX_TILE_X  64
#define MAX_TILE_Y  32

void graphicsCopyImage(struct RenderState* state, void* source, int iw, int ih, int sx, int sy, int dx, int dy, int width, int height, struct Coloru8 color) {
    gDPPipeSync(state->dl++);
    gDPSetCycleType(state->dl++, G_CYC_1CYCLE);
    gDPSetRenderMode(state->dl++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
	gDPSetCombineLERP(state->dl++, TEXEL0, 0, ENVIRONMENT, 0, TEXEL0, 0, ENVIRONMENT, 0, TEXEL0, 0, ENVIRONMENT, 0, TEXEL0, 0, ENVIRONMENT, 0);
    gDPSetTextureLUT(state->dl++, G_TT_NONE);
    gDPSetTexturePersp(state->dl++, G_TP_NONE);
    gDPSetEnvColor(state->dl++, color.r, color.g, color.b, color.a);

    if (dy < 0) {
        height += dy;
        sy -= dy;
        dy = 0;
    }

    if (height <= 0) {
        return;
    }

    int tileXCount = (width + MAX_TILE_X-1) / MAX_TILE_X;
    int tileYCount = (height + MAX_TILE_Y-1) / MAX_TILE_Y;
    int tileX, tileY;

    for (tileX = 0; tileX < tileXCount; ++tileX) {
        int currX = tileX * MAX_TILE_X;
        int tileWidth = width - currX;

        if (tileWidth > MAX_TILE_X) {
            tileWidth = MAX_TILE_X;
        }

        int currDx = dx + currX;

        if (currDx >= SCREEN_WD) {
            break;
        }

        if (currDx + tileWidth < 0) {
            continue;
        }

        if (currDx < 0) {
            tileWidth += currDx;
            currDx = 0;
        }

        if (currDx + tileWidth >= SCREEN_WD) {
            tileWidth = SCREEN_WD - currDx;
        }

        int scaledY = 0;

        for (tileY = 0; tileY < tileYCount; ++tileY) {
            int currY = tileY * MAX_TILE_Y;
            int tileHeight = height - currY;
            
            if (tileHeight > MAX_TILE_Y) {
                tileHeight = MAX_TILE_Y;
            }

            int scaledTileHeight = tileHeight;//SCALE_FOR_PAL(tileHeight);
            
            gDPLoadTextureTile(
                state->dl++,
                K0_TO_PHYS((char*)source + ((sx + currX) + (sy + currY) * iw) * 2),
                G_IM_FMT_RGBA, G_IM_SIZ_16b,
                iw, ih,
                0, 0,
                tileWidth-1, tileHeight-1,
                0,
                G_TX_CLAMP, G_TX_CLAMP,
                G_TX_NOMASK, G_TX_NOMASK,
                G_TX_NOLOD, G_TX_NOLOD
            );
            
            gSPTextureRectangle(
                state->dl++,
                (currDx) << 2, (dy+scaledY) << 2,
                (currDx+tileWidth) << 2, (dy+scaledY+scaledTileHeight) << 2,
                G_TX_RENDERTILE, 
                0, 0, 1 << 10, (tileHeight << 10) / scaledTileHeight
            );

            scaledY += scaledTileHeight;
        }
    }

    gDPPipeSync(state->dl++);
}