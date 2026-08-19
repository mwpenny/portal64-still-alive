#include "image.h"

#include "graphics.h"
#include "system/display.h"

#define MAX_TILE_X  64
#define MAX_TILE_Y  32

void graphicsCopyImage(
    struct RenderState* state,
    void* image,
    int imageWidth,
    int imageHeight,
    int srcX,
    int srcY,
    int srcWidth,
    int srcHeight,
    int screenX,
    int screenY,
    struct Coloru8 color
) {
    gDPPipeSync(state->dl++);
    gDPSetCycleType(state->dl++, G_CYC_1CYCLE);
    gDPSetRenderMode(state->dl++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetCombineMode(state->dl++, G_CC_MODULATERGBA_PRIM, G_CC_MODULATERGBA_PRIM);
    gDPSetTextureLUT(state->dl++, G_TT_NONE);
    gDPSetTexturePersp(state->dl++, G_TP_NONE);
    gDPSetPrimColor(state->dl++, 255, 255, color.r, color.g, color.b, color.a);

    if (screenX < 0) {
        srcWidth += screenX;
        srcX -= screenX;
        screenX = 0;
    }

    if (screenY < 0) {
        srcHeight += screenY;
        srcY -= screenY;
        screenY = 0;
    }

    if (srcWidth <= 0 || srcHeight <= 0) {
        return;
    }

    int tileXCount = (srcWidth + MAX_TILE_X - 1) / MAX_TILE_X;
    int tileYCount = (srcHeight + MAX_TILE_Y - 1) / MAX_TILE_Y;

    for (int tileX = 0; tileX < tileXCount; ++tileX) {
        int currX = tileX * MAX_TILE_X;
        int tileWidth = MIN(srcWidth - currX, MAX_TILE_X);

        int currSrcX = srcX + currX;
        int currScreenX = screenX + currX;

        if (currScreenX >= SCREEN_WD) {
            break;
        } else if (currScreenX + tileWidth >= SCREEN_WD) {
            tileWidth = SCREEN_WD - currScreenX;
        }

        for (int tileY = 0; tileY < tileYCount; ++tileY) {
            int currY = tileY * MAX_TILE_Y;
            int tileHeight = MIN(srcHeight - currY, MAX_TILE_Y);

            int currSrcY = srcY + currY;
            int currScreenY = screenY + currY;

            if (currScreenY >= SCREEN_HT) {
                break;
            } else if (currScreenY + tileHeight >= SCREEN_HT) {
                tileHeight = SCREEN_HT - currScreenY;
            }
            
            gDPLoadTextureTile(
                state->dl++,
                K0_TO_PHYS(image),
                G_IM_FMT_RGBA, G_IM_SIZ_16b,
                imageWidth, imageHeight,
                currSrcX, currSrcY,
                currSrcX + tileWidth - 1, currSrcY + tileHeight - 1,
                0,
                G_TX_CLAMP, G_TX_CLAMP,
                G_TX_NOMASK, G_TX_NOMASK,
                G_TX_NOLOD, G_TX_NOLOD
            );
            
            gSPTextureRectangle(
                state->dl++,
                currScreenX << 2, currScreenY << 2,
                (currScreenX + tileWidth) << 2, (currScreenY + tileHeight) << 2,
                G_TX_RENDERTILE, 
                currSrcX << 5, currSrcY << 5,
                1 << 10, 1 << 10
            );
        }
    }

    gDPPipeSync(state->dl++);
}
