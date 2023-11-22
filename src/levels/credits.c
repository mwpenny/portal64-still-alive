#include "credits.h"

#include "../util/time.h"
#include "./levels.h"
#include "../audio/soundplayer.h"
#include "../build/src/audio/clips.h"
#include "../util/memory.h"
#include "../util/rom.h"
#include "../graphics/image.h"
#include "../build/assets/materials/static.h"
#include "../build/assets/materials/ui.h"
#include "../controls/controller.h"
#include "../font/font.h"
#include "../font/liberation_mono.h"

#define FADE_IN_TIME  1.0f

#define VALVE_IMAGE_SIZE    (sizeof(u16) * VALVE_IMAGE_WIDTH * VALVE_IMAGE_HEIGHT)

void creditsInit(struct Credits* credits) {
    credits->time = 0.0f;

    soundPlayerPlay(SOUNDS_LOOPING_RADIO_MIX, 0.5f, 1.0f, NULL, NULL, SoundTypeMusic);
}

void creditsUpdate(struct Credits* credits) {
    credits->time += FIXED_DELTA_TIME;

    if (controllerGetButtonDown(0, START_BUTTON)) {
        levelQueueLoad(MAIN_MENU, NULL, NULL);
    }
}

void creditsRender(void* data, struct RenderState* renderState, struct GraphicsTask* task) {
    struct Credits* credits = (struct Credits*)data;

    gDPPipeSync(renderState->dl++);
    gDPSetCycleType(renderState->dl++, G_CYC_FILL); 
    gDPSetFillColor(renderState->dl++, 0);
    gDPFillRectangle(renderState->dl++, 0, 0, SCREEN_WD - 1, SCREEN_HT - 1);

    gDPPipeSync(renderState->dl++);
    gDPSetCycleType(renderState->dl++, G_CYC_1CYCLE); 

    struct Coloru8 color = gColorWhite;

    if (credits->time < FADE_IN_TIME) {
        color.a = (u8)(credits->time * (255.0f / FADE_IN_TIME));
    }
    color.g = 200;
    color.b = 30;

    gSPDisplayList(renderState->dl++, ui_material_list[DEFAULT_UI_INDEX]);

    struct FontRenderer* renderer = stackMalloc(sizeof(struct FontRenderer));
    fontRendererLayout(renderer, &gLiberationMonoFont, "THANK YOU FOR PARTICIPATING\nIN THIS\nENRICHMENT CENTER ACTIVITY!!\n\nIt is still in development.", SCREEN_WD);
    renderState->dl = fontRendererBuildGfx(
        renderer, 
        gLiberationMonoImages, 
        35, 
        36, 
        &color, 
        renderState->dl
    );

    fontRendererLayout(renderer, &gLiberationMonoFont, "-----------------------------------------", SCREEN_WD);
    renderState->dl = fontRendererBuildGfx(
        renderer, 
        gLiberationMonoImages, 
        14, 
        12, 
        &color, 
        renderState->dl
    );
    renderState->dl = fontRendererBuildGfx(
        renderer, 
        gLiberationMonoImages, 
        14, 
        SCREEN_HT - 24, 
        &color, 
        renderState->dl
    );

    fontRendererLayout(renderer, &gLiberationMonoFont, "|\n|\n|\n|\n|\n|\n|\n|\n|\n|\n|\n|\n|\n|\n|\n|", SCREEN_WD);
    renderState->dl = fontRendererBuildGfx(
        renderer, 
        gLiberationMonoImages, 
        14, 
        24, 
        &color, 
        renderState->dl
    );
    renderState->dl = fontRendererBuildGfx(
        renderer, 
        gLiberationMonoImages, 
        294, 
        24, 
        &color, 
        renderState->dl
    );

    fontRendererLayout(renderer, &gLiberationMonoFont, "Subscribe", SCREEN_WD);
    renderState->dl = fontRendererBuildGfx(
        renderer, 
        gLiberationMonoImages, 
        70, 
        120, 
        &color, 
        renderState->dl
    );


    fontRendererLayout(renderer, &gLiberationMonoFont, "Support", SCREEN_WD);
    renderState->dl = fontRendererBuildGfx(
        renderer, 
        gLiberationMonoImages, 
        182, 
        120, 
        &color, 
        renderState->dl
    );

    gSPDisplayList(renderState->dl++, ui_material_list[YOUTUBE_QR_INDEX]);
    gSPTextureRectangle(renderState->dl++, 70 << 2, 138 << 2, (70 + 64) << 2, (138 + 64) << 2, G_TX_RENDERTILE, 0, 0, 1 << 9, 1 << 9);

    gSPDisplayList(renderState->dl++, ui_material_list[PATREON_QR_INDEX]);
    gSPTextureRectangle(renderState->dl++, 182 << 2, 138 << 2, (182 + 64) << 2, (138 + 64) << 2, G_TX_RENDERTILE, 0, 0, 1 << 9, 1 << 9);

    gSPDisplayList(renderState->dl++, ui_material_list[CREDITS_ICONS_INDEX]);
    gSPTextureRectangle(renderState->dl++, 30 << 2, 130 << 2, (30 + 32) << 2, (130 + 32) << 2, G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);

    gSPDisplayList(renderState->dl++, ui_material_list[CREDITS_ICONS_INDEX]);
    gSPTextureRectangle(renderState->dl++, 148 << 2, 130 << 2, (148 + 32) << 2, (130 + 32) << 2, G_TX_RENDERTILE, 32 << 5, 0, 1 << 10, 1 << 10);

    stackMallocFree(renderer);
}
