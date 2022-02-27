#include "light.h"

#define LIGHT_COMBINE_MODE        0, 0, 0, PRIMITIVE, 0, 0, 0, ENVIRONMENT

Gfx light_mat[] = {
    gsDPPipeSync(),
    gsSPClearGeometryMode(G_LIGHTING),
    gsDPSetPrimColor(255, 255, 255, 240, 180, 255),
    gsDPSetCombineMode(LIGHT_COMBINE_MODE, LIGHT_COMBINE_MODE),
    gsSPEndDisplayList(),
};