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
#include "../font/dejavusans.h"

#define FADE_IN_TIME  1.0f

#define VALVE_IMAGE_SIZE    (sizeof(u16) * VALVE_IMAGE_WIDTH * VALVE_IMAGE_HEIGHT)

void creditsInit(struct Credits* credits) {
    credits->time = 0.0f;

    soundPlayerPlay(SOUNDS_PORTAL_STILL_ALIVE, 1.0f, 0.5f, NULL, NULL, SoundTypeMusic);
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
    fontRendererLayout(renderer, &gDejaVuSansFont, "Thank you for playing.\nThe rest of the game is still in development.\nFollow the project on YouTube.\nSupport the project on Patreon.", SCREEN_WD);
    renderState->dl = fontRendererBuildGfx(
        renderer, 
        gDejaVuSansImages, 
        (SCREEN_WD - renderer->width) >> 1, 
        (SCREEN_HT - renderer->height) >> 1, 
        &color, 
        renderState->dl
    );

    stackMallocFree(renderer);
}
