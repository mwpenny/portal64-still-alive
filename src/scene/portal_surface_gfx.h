#ifndef __PORTAL_SURFACE_GFX_H__
#define __PORTAL_SURFACE_GFX_H__

#include "portal_surface_generator.h"

struct GfxTraingleIndices {
    u8 indices[3];
};

struct GfxBuilderState {
    short triangleCount;
    struct GfxTraingleIndices* triangles;

    Vtx* vtxCopy;
    Gfx* gfx;
};

struct DisplayListResult {
    Vtx* vtx;
    Gfx* gfx;
};

struct DisplayListResult newGfxFromSurfaceBuilder(struct PortalSurfaceBuilder* surfaceBuilder);

#endif