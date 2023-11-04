#include "intro.h"

#include "../util/time.h"
#include "./levels.h"
#include "../audio/soundplayer.h"
#include "../build/src/audio/clips.h"
#include "../util/memory.h"
#include "../util/rom.h"
#include "../graphics/image.h"
#include "../build/assets/materials/images.h"
#include "../controls/controller.h"

#define INTRO_TIME  9.0f

#define FADE_IN_TIME  1.0f
#define IMAGE_END_TIME 7.0f
#define FADE_OUT_TIME   1.0f

#define VALVE_IMAGE_WIDTH   160
#define VALVE_IMAGE_HEIGHT  120

#define VALVE_IMAGE_SIZE    (sizeof(u16) * VALVE_IMAGE_WIDTH * VALVE_IMAGE_HEIGHT)

void introInit(struct Intro* intro) {
    intro->time = 0.0f;

    intro->valveImage = malloc(VALVE_IMAGE_SIZE);
    romCopy((char*)images_valve_rgba_16b, (char*)intro->valveImage, VALVE_IMAGE_SIZE);

    soundPlayerPlay(SOUNDS_VALVE, 1.0f, 0.5f, NULL, NULL, SoundTypeMusic);
}

void introUpdate(struct Intro* intro) {
    intro->time += FIXED_DELTA_TIME;

    if (intro->time > INTRO_TIME || controllerGetButtonDown(0, START_BUTTON)) {
        levelQueueLoad(MAIN_MENU, NULL, NULL);
    }
}

void introRender(void* data, struct RenderState* renderState, struct GraphicsTask* task) {
    struct Intro* intro = (struct Intro*)data;

    struct Coloru8 fadeColor = gColorWhite;

    if (intro->time < FADE_IN_TIME) {
        fadeColor.a = (u8)((255.0f / FADE_IN_TIME) * intro->time);
    } else if (intro->time > IMAGE_END_TIME) {
        fadeColor.a = 0;
    } else if (intro->time > IMAGE_END_TIME - FADE_OUT_TIME) {
        fadeColor.a = (u8)((255.0f / FADE_OUT_TIME) * (IMAGE_END_TIME - intro->time));
    }

    gDPPipeSync(renderState->dl++);
    gDPSetCycleType(renderState->dl++, G_CYC_FILL); 
    gDPSetFillColor(renderState->dl++, 0);
    gDPFillRectangle(renderState->dl++, 0, 0, SCREEN_WD - 1, SCREEN_HT - 1);

    gDPPipeSync(renderState->dl++);
    gDPSetCycleType(renderState->dl++, G_CYC_1CYCLE); 
    graphicsCopyImage(
        renderState, 
        intro->valveImage, 
        VALVE_IMAGE_WIDTH, VALVE_IMAGE_HEIGHT, 
        0, 0, 
        (SCREEN_WD - VALVE_IMAGE_WIDTH) / 2, (SCREEN_HT - VALVE_IMAGE_HEIGHT) / 2, 
        VALVE_IMAGE_WIDTH, VALVE_IMAGE_HEIGHT, 
        fadeColor
    );
}
