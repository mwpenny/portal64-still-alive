#include "system/display.h"

#include "rsp_scheduler_libultra.h"

#include <ultra64.h>

static u8 displayGetTargetMode(int interlaced) {
#if HIGH_RES
    switch (osTvType) {
        case OS_TV_PAL:
            return interlaced ? OS_VI_PAL_HPF1 : OS_VI_PAL_HPN1;
        case OS_TV_MPAL:
            return interlaced ? OS_VI_MPAL_HPF1 : OS_VI_MPAL_HPN1;
        case OS_TV_NTSC:
        default:
            return interlaced ? OS_VI_NTSC_HPF1 : OS_VI_NTSC_HPN1;
    }
#else
    switch (osTvType) {
        case OS_TV_PAL:
            return interlaced ? OS_VI_PAL_LPF1 : OS_VI_PAL_LPN1;
        case OS_TV_MPAL:
            return interlaced ? OS_VI_MPAL_LPF1 : OS_VI_MPAL_LPN1;
        case OS_TV_NTSC:
        default:
            return interlaced ? OS_VI_NTSC_LPF1 : OS_VI_NTSC_LPN1;
    }
#endif
}

static void displaySetFeatures() {
    osViSetSpecialFeatures(
        OS_VI_GAMMA_OFF         |
        OS_VI_GAMMA_DITHER_OFF  |
        OS_VI_DIVOT_OFF         |
        OS_VI_DITHER_FILTER_OFF
    );
}

void displayInit(int interlaced) {
    rspSchedulerInit(displayGetTargetMode(interlaced));
    displaySetFeatures();
    osViBlack(1);
}

void displaySetMode(int interlaced) {
    osViSetMode(&osViModeTable[displayGetTargetMode(interlaced)]);
    displaySetFeatures();
}

void displayClearScreen() {
    osViBlack(1);
}

int displayGetFPS() {
    return (osTvType == OS_TV_PAL) ? 50 : 60;
}

uint16_t* displayGetCurrentFramebuffer() {
    return osViGetCurrentFramebuffer();
}
