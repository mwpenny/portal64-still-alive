#include <ultra64.h>

#define SHADOW_COMBINE_MODE        PRIMITIVE, 0, SHADE, 0, 0, 0, 0, ENVIRONMENT

Gfx shadow_caster_mat[] = {
    gsDPPipeSync(),
    gsSPSetGeometryMode(G_LIGHTING | G_SHADE),
    gsDPSetPrimColor(255, 255, 32, 64, 200, 255),
    gsDPSetCombineMode(SHADOW_COMBINE_MODE, SHADOW_COMBINE_MODE),
    gsSPEndDisplayList(),
};