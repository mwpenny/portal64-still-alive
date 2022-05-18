#include "TextureFormats.h"

const char* G_IM_FMT_NAMES[] = {
    "G_IM_FMT_RGBA",
    "G_IM_FMT_YUV",
    "G_IM_FMT_CI",
    "G_IM_FMT_I",
    "G_IM_FMT_IA",
};

const char* nameForImageFormat(G_IM_FMT format) {
    return G_IM_FMT_NAMES[(int)format];
}

const char* G_IM_SIZ_NAMES[] = {
    "G_IM_SIZ_4b",
    "G_IM_SIZ_8b",
    "G_IM_SIZ_16b",
    "G_IM_SIZ_32b",
};

const char* nameForImageSize(G_IM_SIZ size) {
    return G_IM_SIZ_NAMES[(int)size];
}

bool G_IM_SUPPORTED[5][4] = {
    {false, false, true, true},
    {false, false, true, false},
    {true, true, false, false},
    {true, true, true, false},
    {true, true, false, false},
};

bool isImageFormatSupported(G_IM_FMT format, G_IM_SIZ size) {
    return G_IM_SUPPORTED[(int)format][(int)size];
}

const int G_IM_SIZ_SIZES[] = {
    4,
    8,
    16,
    32,
};

int bitSizeforSiz(G_IM_SIZ input) {
    return G_IM_SIZ_SIZES[(int)input];
}