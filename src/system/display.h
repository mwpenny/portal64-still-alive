#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include <stdint.h>

#define HIGH_RES 0

#if HIGH_RES
    #define SCREEN_WD   640
    #define SCREEN_HT   480
#else
    #define SCREEN_WD   320
    #define SCREEN_HT   240
#endif

void displayInit(int interlaced);
void displaySetMode(int interlaced);
void displayClearScreen();

int displayGetFPS();
uint16_t* displayGetCurrentFramebuffer();

#endif
