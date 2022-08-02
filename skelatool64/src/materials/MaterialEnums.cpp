#include "MaterialEnums.h"

const char* gGeometryModeNames[GEOMETRY_MODE_COUNT] = {
    "G_ZBUFFER",
    "G_SHADE",
    "G_TEXTURE_ENABLE",
    "G_SHADING_SMOOTH",
    "G_CULL_FRONT",
    "G_CULL_BACK",
    "G_FOG",
    "G_LIGHTING",
    "G_TEXTURE_GEN",
    "G_TEXTURE_GEN_LINEAR",
    "G_LOD",
    "G_CLIPPING",
};

const char* gCycleTypeNames[(int)CycleType::Count] = {
    "Unknown",
    "G_CYC_1CYCLE",
    "G_CYC_2CYCLE",
    "G_CYC_COPY",
    "G_CYC_FILL",
};

const char* gColorCombineSourceNames[(int)ColorCombineSource::Count] = {
    "COMBINED",
    "TEXEL0",
    "TEXEL1",
    "PRIMITIVE",
    "SHADE",
    "ENVIRONMENT",
    "CENTER",
    "SCALE",
    "COMBINED_ALPHA",
    "TEXEL0_ALPHA",
    "TEXEL1_ALPHA",
    "PRIMITIVE_ALPHA",
    "SHADE_ALPHA",
    "ENVIRONMENT_ALPHA",
    "LOD_FRACTION",
    "PRIM_LOD_FRAC",
    "NOISE",
    "K4",
    "K5",
    "1",
    "0",
};

bool gCanUseColorCombineSource[4][(int)ColorCombineSource::Count] = {
    {true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, true, false, false, true, true},
    {true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, true, false, false, true},
    {true, true, true, true, true, true, false, true, true, true, true, true, true, true, true, true, false, false, true, false, true},
    {true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true},
};

bool canUseColorCombineSource(int offset, ColorCombineSource source) {
    return gCanUseColorCombineSource[offset][(int)source];
}


const char* gAlphaCombineSourceNames[(int)AlphaCombineSource::Count] = {
    "COMBINED",
    "TEXEL0",
    "TEXEL1",
    "PRIMITIVE",
    "SHADE",
    "ENVIRONMENT",
    "LOD_FRACTION",
    "PRIM_LOD_FRAC",
    "1",
    "0",
};

bool gCanUseAlphaCombineSources[4][(int)AlphaCombineSource::Count] = {
    {true, true, true, true, true, true, false, false, true, true},
    {true, true, true, true, true, true, false, false, true, true},
    {false, true, true, true, true, true, true, true, false, true},
    {true, true, true, true, true, true, false, false, true, true},
};

bool canUseAlphaCombineSource(int offset, ColorCombineSource source) {
    return gCanUseAlphaCombineSources[offset][(int)source];
}

const char* gPipelineModeNames[(int)PipelineMode::Count] = {
    "Unknown",
    "G_PM_1PRIMITIVE",
    "G_PM_NPRIMITIVE",
};

const char* gPerspectiveModeNames[(int)PerspectiveMode::Count] = {
    "Unknown",
    "G_TP_NONE",
    "G_TP_PERSP",
};

const char* gTextureDetailNames[(int)TextureDetail::Count] = {
    "Unknown",
    "G_TD_CLAMP",
    "G_TD_SHARPEN",
    "G_TD_DETAIL",
};

const char* gTextureLODNames[] = {
    "Unknown",
    "G_TL_TILE",
    "G_TL_LOD",
};

const char* gTextureLUTNames[] = {
    "Unknown",
    "G_TT_NONE",
    "G_TT_RGBA16",
    "G_TT_IA16",
};

const char* gTextureFilterNames[] = {
    "Unknown",
    "G_TF_POINT",
    "G_TF_AVERAGE",
    "G_TF_BILERP",
};

const char* gTextureConvertNames[] = {
    "Unknown",
    "G_TC_CONV",
    "G_TC_FILTCONV",
    "G_TC_FILT",
};

const char* gCombineKeyNames[] = {
    "Unknown",
    "G_CK_NONE",
    "G_CK_KEY",
};

const char* gCotherDitherNames[] = {
    "Unknown",
    "G_CD_MAGICSQ",
    "G_CD_BAYER",
    "G_CD_NOISE",
    "G_CD_DISABLE",
};

const char* gAlphaDitherNames[] = {
    "Unknown",
    "G_AD_PATTERN",
    "G_AD_NOTPATTERN",
    "G_AD_NOISE",
    "G_AD_DISABLE",
};

const char* gAlphaCompareNames[] = {
    "Unknown",
    "G_AC_NONE",
    "G_AC_THRESHOLD",
    "G_AC_DITHER",
};

const char* gDepthSourceNames[] = {
    "Unknown",
    "G_ZS_PIXEL",
    "G_ZS_PRIM",
};

bool gCanUseAlphaBlendSource[2][7] = {
    {false, false, true, true, true, false, true},
    {true, true, false, false, false, true, true},
};