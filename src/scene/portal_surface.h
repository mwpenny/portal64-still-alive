#ifndef __PORTAL_SURFACE_H__
#define __PORTAL_SURFACE_H__

#include <ultra64.h>

#include "math/transform.h"
#include "math/plane.h"

#define FIXED_POINT_PRECISION   8
#define FIXED_POINT_SCALAR      (1 << FIXED_POINT_PRECISION)

#define VECTOR2s16_AS_ARRAY(vector) ((s16*)(vector))

struct Vector2s16 {
    s16 x;
    s16 y;
};

struct SurfaceEdge {
    u8 aIndex;
    u8 bIndex;
};

struct SurfaceFace {
    u8 aIndex;
    u8 bIndex;
    u8 cIndex;
};

struct PortalSurface {
    struct Vector2s16* vertices;
    // first sideCount edges are on the side
    struct SurfaceEdge* edges;
    struct SurfaceFace* triangles;

    u8 sideCount;
    u8 edgeCount;
    u8 triangleCount;

    struct Vector3 right;
    struct Vector3 up;
    struct Vector3 corner;
};

struct PortalSurfaceMapping {
    u8 minPortalIndex;
    u8 maxPortalIndex;
};

int portalSurfaceIsInside(struct PortalSurface* surface, struct Transform* portalAt);

int portalSurfaceGenerate(struct PortalSurface* surface, struct Transform* portalAt, Vtx* vertices, Gfx* triangles);

#endif