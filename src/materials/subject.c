#include "subject.h"

#define SUBJECT_COMBINE_MODE PRIMITIVE, 0, SHADE, 0, 0, 0, 0, ENVIRONMENT

Gfx subject_mat[] = {
    gsDPPipeSync(),
    gsSPSetGeometryMode(G_LIGHTING | G_SHADE),
    gsDPSetPrimColor(255, 255, 0xF2, 0x66, 0x27, 255),
    gsDPSetCombineMode(SUBJECT_COMBINE_MODE, SUBJECT_COMBINE_MODE),
    gsSPEndDisplayList(),
};