#ifndef __PORTAL_SURFACE_H__
#define __PORTAL_SURFACE_H__

#include <ultra64.h>

#include "math/transform.h"
#include "math/plane.h"
#include "math/vector2s16.h"
#include "../levels/level_definition.h"

#define FIXED_POINT_PRECISION   8
#define FIXED_POINT_SCALAR      (1 << FIXED_POINT_PRECISION)

#define NO_EDGE_CONNECTION      0xFF

struct SurfaceEdge {
    u8 aIndex;
    u8 bIndex;
    u8 nextEdge;
    u8 prevEdge;
    u8 nextEdgeReverse;
    u8 prevEdgeReverse;
};

struct PortalSurface {
    struct Vector2s16* vertices;
    struct SurfaceEdge* edges;
    
    u8 edgeCount;
    u8 vertexCount;
    u8 shouldCleanup;

    struct Vector3 right;
    struct Vector3 up;
    struct Vector3 corner;

    Vtx* gfxVertices;
    Gfx* triangles;
};

struct PortalSurfaceMappingRange {
    u8 minPortalIndex;
    u8 maxPortalIndex;
};

enum PortalSurfaceReplacementFlags {
    PortalSurfaceReplacementFlagsIsEnabled = (1 << 0),
};

struct PortalSurfaceReplacement {
    struct PortalSurface previousSurface;
    short flags;
    short staticIndex;
    short portalSurfaceIndex;
    short roomIndex;
};

int portalSurfaceAreBothOnSameSurface();
int portalSurfaceShouldSwapOrder(int portalToMove);
int portalSurfaceStaticIndexForReplacement(int portalIndex);

int portalSurfaceIsInside(struct PortalSurface* surface, struct Transform* portalAt);

int portalSurfaceGenerate(struct PortalSurface* surface, int surfaceIndex, struct Transform* portalAt, int portalIndex, struct Transform* otherPortalAt, struct PortalSurface* newSurface);

void portalSurfaceCleanup(struct PortalSurface* portalSurface);

int portalSurfaceAdjustPosition(struct PortalSurface* surface, struct Transform* portalAt, struct Vector2s16* output, struct Vector2s16* outlineLoopOutput);

struct PortalSurface* portalSurfaceGetOriginalSurface(int portalSurfaceIndex, int portalIndex);
void portalSurfaceInverse(struct PortalSurface* surface, struct Vector2s16* input, struct Vector3* output);

struct PortalSurface* portalSurfaceReplace(int portalSurfaceIndex, int roomIndex, int portalIndex, struct PortalSurface* with);
void portalSurfaceRevert(int portalIndex);
void portalSurfaceCheckCleanupQueue();


#endif