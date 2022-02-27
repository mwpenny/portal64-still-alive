#include "point_light_rendered.h"

#include "point_light_rendered_tex.inc.h"

#define POINT_LIGHT_COMBINE_MODE    0, 0, 0, TEXEL0, 0, 0, 0, ENVIRONMENT 

Gfx point_light_mat[] = {
	gsDPPipeSync(),
    gsSPClearGeometryMode(G_LIGHTING | G_SHADE),
    gsDPSetCombineMode(POINT_LIGHT_COMBINE_MODE, POINT_LIGHT_COMBINE_MODE),
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, 1),
    gsDPSetTextureLUT(G_TT_NONE),
    gsDPSetTextureFilter(G_TF_BILERP),
    gsDPTileSync(),
	gsDPSetTextureImage(G_IM_FMT_I, G_IM_SIZ_8b, 64, point_light_rendered_tex),
    gsDPSetTile(
        G_IM_FMT_I, G_IM_SIZ_8b, 
        8, 0, G_TX_LOADTILE, 0, 
        G_TX_MIRROR | G_TX_CLAMP, 6, 0,
        G_TX_MIRROR | G_TX_CLAMP, 6, 0
    ),
    gsDPLoadSync(),
    gsDPLoadTile(G_TX_LOADTILE, 0, 0, 252, 252),
    gsDPPipeSync(),
    gsDPSetTile(
        G_IM_FMT_I, G_IM_SIZ_8b, 
        8, 0, G_TX_RENDERTILE, 0, 
        G_TX_MIRROR | G_TX_CLAMP, 6, 0,
        G_TX_MIRROR | G_TX_CLAMP, 6, 0
    ),
    gsDPSetTileSize(G_TX_RENDERTILE, 0, 0, 508, 508),
    gsSPEndDisplayList(),
};