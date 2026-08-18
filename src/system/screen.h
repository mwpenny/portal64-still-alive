#ifndef __SCREEN_H__
#define __SCREEN_H__

#include <stdint.h>

#define HIGH_RES 0

#if HIGH_RES
    #define SCREEN_WD   640
    #define SCREEN_HT   480
#else
    #define SCREEN_WD   320
    #define SCREEN_HT   240
#endif

void screenInit(int interlaced);
void screenSetMode(int interlaced);
int screenGetFPS();
uint16_t* screenGetCurrentFramebuffer();

#endif
