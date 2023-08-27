#ifndef __PORTAL_SURFACE_GENERATOR_H__
#define __PORTAL_SURFACE_GENERATOR_H__

#include "portal_surface.h"

#define PORTAL_SURFACE_OVERLAP  0x10000

enum SurfaceEdgeFlags {
    SurfaceEdgeFlagsUsed = (1 << 0),
    SurfaceEdgeFlagsTriangulated = (1 << 1),
    SurfaceEdgeFlagsFilled = (1 << 2),
};

struct PortalSurfaceBuilder {
    struct PortalSurface* original;

    struct Vector2s16* vertices;
    struct SurfaceEdge* edges;
    short currentVertex;
    short currentEdge;

    short checkForEdgeReuse;

    u8* isLoopEdge;
    u8* edgeFlags;
    u8* originalEdgeIndex;

    u8 edgeOnSearchLoop;
    union {
        // set when hasEdge is true
        u8 cuttingEdge;
        // set when hasEdge is false
        int startingPoint;
    };
    short hasEdge;
    short hasConnected;

    Vtx* gfxVertices;
};

int portalSurfacePokeHole(struct PortalSurface* surface, struct Vector2s16* loop, struct PortalSurface* result);
int portalSurfaceHasFlag(struct PortalSurfaceBuilder* surfaceBuilder, int edgeIndex, enum SurfaceEdgeFlags value);
void portalSurfaceSetFlag(struct PortalSurfaceBuilder* surfaceBuilder, int edgeIndex, enum SurfaceEdgeFlags value);
struct SurfaceEdge* portalSurfaceGetEdge(struct PortalSurfaceBuilder* surfaceBuilder, int edgeIndex);

int portalSurfaceNextEdge(struct PortalSurfaceBuilder* surfaceBuilder, int edgeIndex);
int portalSurfacePrevEdge(struct PortalSurfaceBuilder* surfaceBuilder, int edgeIndex);

#endif