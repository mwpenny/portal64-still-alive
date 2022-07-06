#ifndef __PORTAL_SURFACE_GENERATOR_H__
#define __PORTAL_SURFACE_GENERATOR_H__

#include "portal_surface.h"

#define PORTAL_SURFACE_OVERLAP  0x10000

#define SB_GET_NEXT_EDGE(surfaceEdge, isReverse) ((isReverse) ? (surfaceEdge)->nextEdgeReverse : (surfaceEdge)->nextEdge)
#define SB_GET_PREV_EDGE(surfaceEdge, isReverse) ((isReverse) ? (surfaceEdge)->prevEdgeReverse : (surfaceEdge)->prevEdge)

#define SB_GET_CURRENT_POINT(surfaceEdge, isReverse) ((isReverse) ? (surfaceEdge)->bIndex : (surfaceEdge->aIndex))
#define SB_GET_NEXT_POINT(surfaceEdge, isReverse) ((isReverse) ? (surfaceEdge)->aIndex : (surfaceEdge->bIndex))

#define SB_SET_NEXT_EDGE(surfaceEdge, isReverse, value) do { if (isReverse) (surfaceEdge)->nextEdgeReverse = (value); else (surfaceEdge)->nextEdge = (value); } while (0)
#define SB_SET_PREV_EDGE(surfaceEdge, isReverse, value) do { if (isReverse) (surfaceEdge)->prevEdgeReverse = (value); else (surfaceEdge)->prevEdge = (value); } while (0)

#define SB_SET_CURRENT_POINT(surfaceEdge, isReverse, value) do { if (isReverse) (surfaceEdge)->bIndex = (value); else (surfaceEdge)->aIndex = (value); } while (0)
#define SB_SET_NEXT_POINT(surfaceEdge, isReverse, value) do { if (isReverse) (surfaceEdge)->aIndex = (value); else (surfaceEdge)->bIndex = (value); } while (0)


struct SurfaceEdgeWithSide {
    int edgeIndex;
    int isReverse;
};

#define SB_ORIGINAL_EDGE_TO_EDGE_WITH_SIDE(originalEdge, edge) do { (edge)->edgeIndex = (originalEdge) & 0x7F; (edge)->isReverse = ((originalEdge) & 0x80) != 0; } while (0)
#define SB_PACK_ORIGINALEDGE(edge)  (((edge)->edgeIndex & 0x7F) | ((edge)->isReverse) ? 0x80 : 0x00)

struct OriginalEdgeMapping {
    u8 originalEdge;
    u8 originalEdgeReverse;
};

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
    struct OriginalEdgeMapping* originalEdgeIndex;

    struct SurfaceEdgeWithSide edgeOnSearchLoop;
    union {
        // set when hasEdge is true
        struct SurfaceEdgeWithSide cuttingEdge;
        // set when hasEdge is false
        int startingPoint;
    };
    short hasEdge;
    short hasConnected;

    Vtx* gfxVertices;
};

int portalSurfacePokeHole(struct PortalSurface* surface, struct Vector2s16* loop, struct PortalSurface* result);
int portalSurfaceHasFlag(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* edge, enum SurfaceEdgeFlags value);
void portalSurfaceSetFlag(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* edge, enum SurfaceEdgeFlags value);
struct SurfaceEdge* portalSurfaceGetEdge(struct PortalSurfaceBuilder* surfaceBuilder, int edgeIndex);

void portalSurfaceNextEdge(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* currentEdge, struct SurfaceEdgeWithSide* nextEdge);
void portalSurfacePrevEdge(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* currentEdge, struct SurfaceEdgeWithSide* prevEdge);

#endif