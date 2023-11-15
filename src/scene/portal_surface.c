#include "portal_surface.h"

#include "portal.h"
#include "math/mathf.h"
#include "math/vector2.h"
#include <math.h>
#include "portal_surface_generator.h"
#include "../levels/level_definition.h"
#include "../levels/levels.h"
#include "../util/memory.h"

#define MAX_PENDING_PORTAL_CLEANUP  4

#define PORTAL_HOLE_SCALE_X  0.945f
#define PORTAL_HOLE_SCALE_Y  0.795f

struct Vector3 gPortalOutlineWorld[PORTAL_LOOP_SIZE] = {
    {-0.353553f * PORTAL_HOLE_SCALE_X, 0.707107f * PORTAL_HOLE_SCALE_Y, 0.0f},
    {-0.5f * PORTAL_HOLE_SCALE_X, 0.0f, 0.0f},
    {-0.353553f * PORTAL_HOLE_SCALE_X, -0.707107f * PORTAL_HOLE_SCALE_Y, 0.0f},
    {0.0f, -1.0f * PORTAL_HOLE_SCALE_Y, 0.0f},
    {0.353553f * PORTAL_HOLE_SCALE_X, -0.707107f * PORTAL_HOLE_SCALE_Y, 0.0f},
    {0.5f * PORTAL_HOLE_SCALE_X, 0.0f, 0.0f},
    {0.353553f * PORTAL_HOLE_SCALE_X, 0.707107f * PORTAL_HOLE_SCALE_Y, 0.0f},
    {0.0f, 1.0f * PORTAL_HOLE_SCALE_Y, 0.0f},
};

struct PortalSurface gPortalSurfaceCleanupQueue[MAX_PENDING_PORTAL_CLEANUP];
int gPortalSurfaceNextToWrite;

void portalSurfaceCheckCleanupQueue() {
    for (int searchIterator = 0; searchIterator < MAX_PENDING_PORTAL_CLEANUP; ++searchIterator) {
        struct PortalSurface* surface = &gPortalSurfaceCleanupQueue[searchIterator];

        if (surface->shouldCleanup == 1) {
            surface->shouldCleanup = 0;

            free(surface->vertices);
            free(surface->edges);
            free(surface->gfxVertices);
            free(surface->triangles);
        } else if (surface->shouldCleanup) {
            --surface->shouldCleanup;
        }
    }
}

void portalSurfaceCleanupQueueInit() {
    for (int searchIterator = 0; searchIterator < MAX_PENDING_PORTAL_CLEANUP; ++searchIterator) {
        gPortalSurfaceCleanupQueue[searchIterator].shouldCleanup = 0;
    }
}

struct PortalSurfaceReplacement gPortalSurfaceReplacements[2];


int portalSurfaceGetSurfaceIndex(int portalIndex) {
    if (gPortalSurfaceReplacements[portalIndex].flags & PortalSurfaceReplacementFlagsIsEnabled) {
        return gPortalSurfaceReplacements[portalIndex].portalSurfaceIndex;
    }

    return -1;
}

int portalSurfaceShouldMove(int portalIndex, int portalSurfaceIndex, float portalScale) {
    struct PortalSurfaceReplacement* replacement = &gPortalSurfaceReplacements[portalIndex];

    int noPortalSurfaceRequested = portalSurfaceIndex == -1;
    int noPortalSurfacePresent = !(replacement->flags & PortalSurfaceReplacementFlagsIsEnabled);

    if (noPortalSurfaceRequested && noPortalSurfacePresent) {
        return 0;
    }

    if (noPortalSurfaceRequested != noPortalSurfacePresent) {
        return 1;
    }

    return portalSurfaceIndex != replacement->portalSurfaceIndex || portalScale != replacement->portalScale;
}

void portalSurfaceReplacementRevert(struct PortalSurfaceReplacement* replacement) {
    if (!(replacement->flags & PortalSurfaceReplacementFlagsIsEnabled)) {
        return;
    }

    gCurrentLevel->staticContent[replacement->staticIndex].displayList = replacement->previousSurface.triangles;
    portalSurfaceCleanup(&gCurrentLevel->portalSurfaces[replacement->portalSurfaceIndex]);
    gCurrentLevel->portalSurfaces[replacement->portalSurfaceIndex] = replacement->previousSurface;
    replacement->flags = 0;
    replacement->portalScale = 0.0f;
}

void portalSurfacePreSwap(int portalToMove) {
    portalSurfaceReplacementRevert(&gPortalSurfaceReplacements[1 - portalToMove]);
    portalSurfaceReplacementRevert(&gPortalSurfaceReplacements[portalToMove]);
}

int portalSurfaceStaticIndexForReplacement(int portalIndex) {
    return gPortalSurfaceReplacements[portalIndex].staticIndex;
}

struct PortalSurface* portalSurfaceGetOriginalSurface(int portalSurfaceIndex, int portalIndex) {
    struct PortalSurfaceReplacement* replacement = &gPortalSurfaceReplacements[portalIndex];

    // TODO swap portals if they are on the same surface
    if ((replacement->flags & PortalSurfaceReplacementFlagsIsEnabled) != 0 && replacement->portalSurfaceIndex == portalSurfaceIndex) {
        return &replacement->previousSurface;
    } else {
        return &gCurrentLevel->portalSurfaces[portalSurfaceIndex];
    }
}

struct PortalSurface* portalSurfaceReplace(int portalSurfaceIndex, int roomIndex, int portalIndex, float portalScale, struct PortalSurface* with) {
    int staticIndex = -1;

    struct PortalSurfaceReplacement* replacement = &gPortalSurfaceReplacements[portalIndex];

    if (replacement->flags & PortalSurfaceReplacementFlagsIsEnabled) {
        portalSurfaceReplacementRevert(replacement);
    }

    struct Rangeu16 range = gCurrentLevel->roomStaticMapping[roomIndex];

    struct PortalSurface* existing = &gCurrentLevel->portalSurfaces[portalSurfaceIndex];

    for (staticIndex = range.min; staticIndex < (int)range.max; ++staticIndex) {
        if (gCurrentLevel->staticContent[staticIndex].displayList == existing->triangles) {
            break;
        }
    }

    if (staticIndex == range.max) {
        portalSurfaceCleanup(with);
        return NULL;
    }

    replacement->previousSurface = *existing;
    replacement->flags = PortalSurfaceReplacementFlagsIsEnabled;
    replacement->staticIndex = staticIndex;
    replacement->portalSurfaceIndex = portalSurfaceIndex;
    replacement->roomIndex = roomIndex;
    replacement->portalScale = portalScale;

    gCurrentLevel->staticContent[staticIndex].displayList = with->triangles;
    gCurrentLevel->portalSurfaces[portalSurfaceIndex] = *with;

    return &gCurrentLevel->portalSurfaces[portalSurfaceIndex];
}

void portalSurfaceRevert(int portalIndex) {
    portalSurfaceReplacementRevert(&gPortalSurfaceReplacements[portalIndex]);
}

void portalSurface2DPoint(struct PortalSurface* surface, struct Vector3* at, struct Vector2s16* output) {
    struct Vector3 offset;
    vector3Sub(at, &surface->corner, &offset);
    output->x = (short)(vector3Dot(&surface->right, &offset) * FIXED_POINT_SCALAR);
    output->y = (short)(vector3Dot(&surface->up, &offset) * FIXED_POINT_SCALAR);
}

void portalSurfaceInverse(struct PortalSurface* surface, struct Vector2s16* input, struct Vector3* output) {
    vector3AddScaled(&surface->corner, &surface->up, (float)input->y / FIXED_POINT_SCALAR, output);
    vector3AddScaled(output, &surface->right, (float)input->x / FIXED_POINT_SCALAR, output);
}

int portalSurfaceIsInside(struct PortalSurface* surface, struct Transform* portalAt) {
    int intersectionCount = 0;

    struct Vector3 portalForward;
    quatMultVector(&portalAt->rotation, &gForward, &portalForward);

    struct Vector3 surfaceNormal;
    vector3Cross(&surface->up, &surface->right, &surfaceNormal);

    float normal = fabsf(vector3Dot(&portalForward, &surfaceNormal));

    if (normal < 0.7f) {
        return 0;
    }

    struct Vector2s16 portalPosition;
    portalSurface2DPoint(surface, &portalAt->position, &portalPosition);

    for (int i = 0; i < surface->edgeCount; ++i) {
        struct SurfaceEdge* edge = &surface->edges[i];

        // only check edges that are one sided
        if (edge->reverseEdge != NO_EDGE_CONNECTION) {
            continue;
        }

        struct Vector2s16 a = surface->vertices[edge->pointIndex];
        struct Vector2s16 b = surface->vertices[surface->edges[edge->nextEdge].pointIndex];

        if ((portalPosition.x - a.x) * (portalPosition.x - b.x) > 0) {
            // edge is to the left or to the right of the portal
            continue;
        }

        // check for the vertical line
        if (a.x == b.x) {
            if (a.y > portalPosition.y && b.y > portalPosition.y) {
                // line is above portal
                ++intersectionCount;
            } else if ((portalPosition.y - a.y) * (portalPosition.y - b.y) > 0) {
                // portal is on an edge, exit early
                return 0;
            }

            continue;
        }

        int yIntersection = (int)(a.y - b.y) * (portalPosition.x - b.x) / (a.x - b.x) + b.y;

        if (yIntersection > portalPosition.y) {
            ++intersectionCount;
        } else if (yIntersection == portalPosition.y) {
            // portal is on an edge exit early
            return 0;
        }
    }

    return intersectionCount % 2;
}

#define MAX_POS_ADJUST_ITERATIONS   3

#define PORTAL_EDGE_PADDING         3

int portalSurfaceAdjustPosition(struct PortalSurface* surface, struct Transform* portalAt, struct Vector2s16* output, struct Vector2s16* outlineLoopOutput) {
    struct Vector2s16 minPortal;
    struct Vector2s16 maxPortal;

    portalSurface2DPoint(surface, &portalAt->position, output);

    struct Vector3 portalNormal;
    quatMultVector(&portalAt->rotation, &gForward, &portalNormal);

    struct Vector3 surfaceNormal;
    vector3Cross(&surface->right, &surface->up, &surfaceNormal);

    int shouldReverse = vector3Dot(&portalNormal, &surfaceNormal) > 0.0f;

    struct Vector2s16 startingPoint = *output;

    minPortal = *output;
    maxPortal = *output;

    for (int i = 0; i < PORTAL_LOOP_SIZE; ++i) {        
        struct Vector3 transformedPoint;
        transformPoint(portalAt, &gPortalOutlineWorld[shouldReverse ? (PORTAL_LOOP_SIZE - 1) - i : i], &transformedPoint);
        portalSurface2DPoint(surface, &transformedPoint, &outlineLoopOutput[i]);

        minPortal.x = MIN(minPortal.x, outlineLoopOutput[i].x);
        minPortal.y = MIN(minPortal.y, outlineLoopOutput[i].y);

        maxPortal.x = MAX(maxPortal.x, outlineLoopOutput[i].x);
        maxPortal.y = MAX(maxPortal.y, outlineLoopOutput[i].y);
    }


    struct Vector2s16 halfSize;
    halfSize.x = ((maxPortal.x - minPortal.x) >> 1) + PORTAL_EDGE_PADDING * 2;
    halfSize.y = ((maxPortal.y - minPortal.y) >> 1) + PORTAL_EDGE_PADDING * 2;

    int iteration = 0;

    for (iteration = 0; iteration < MAX_POS_ADJUST_ITERATIONS; ++iteration) {
        int minOverlap = PORTAL_SURFACE_OVERLAP;
        struct Vector2s16 minOverlapOffset;

        struct Vector2s16 portalMin;
        portalMin.x = output->x - halfSize.x;
        portalMin.y = output->y - halfSize.y;
        struct Vector2s16 portalMax;
        portalMax.x = output->x + halfSize.x;
        portalMax.y = output->y + halfSize.y;

        for (int i = 0; i < surface->edgeCount; ++i) {
            struct SurfaceEdge* edge = &surface->edges[i];

            // only check edges that are one sided
            if (edge->reverseEdge != NO_EDGE_CONNECTION) {
                continue;
            }

            struct Vector2s16 a = surface->vertices[edge->pointIndex];
            struct Vector2s16 b = surface->vertices[surface->edges[edge->nextEdge].pointIndex];

            struct Vector2s16 edgeMin;
            edgeMin.x = MIN(a.x, b.x);
            edgeMin.y = MIN(a.y, b.y);

            struct Vector2s16 edgeMax;
            edgeMax.x = MAX(a.x, b.x);
            edgeMax.y = MAX(a.y, b.y);

            // the line bounding box doesn't intersect the portal bounding box
            if (portalMax.x <= edgeMin.x || portalMin.x >= edgeMax.x ||
                portalMax.y <= edgeMin.y || portalMin.y >= edgeMax.y) {
                continue;
            }

            struct Vector2s16 offset;
            int distance = portalMax.x - edgeMin.x;
            offset.x = -distance;
            offset.y = 0;

            int distanceCheck = edgeMax.x - portalMin.x;

            if (distanceCheck < distance) {
                distance = distanceCheck;
                offset.x = distance;
                offset.y = 0;
            }

            distanceCheck = portalMax.y - edgeMin.y;

            if (distanceCheck < distance) {
                distance = distanceCheck;
                offset.x = 0;
                offset.y = -distance;
            }

            distanceCheck = edgeMax.y - portalMin.y;

            if (distanceCheck < distance) {
                distance = distanceCheck;
                offset.x = 0;
                offset.y = distance;
            }

            if (distance < minOverlap) {
                minOverlap = distance;
                minOverlapOffset = offset;
            }
        }

        if (minOverlap == PORTAL_SURFACE_OVERLAP) {
            break;
        }

        output->x += minOverlapOffset.x;
        output->y += minOverlapOffset.y;        
    }

    // if the output position moved then adjust the loop
    if (startingPoint.equalTest != output->equalTest) {
        struct Vector2s16 offset;
        vector2s16Sub(output, &startingPoint, &offset);

        for (int i = 0; i < PORTAL_LOOP_SIZE; ++i) {
            vector2s16Add(&outlineLoopOutput[i], &offset, &outlineLoopOutput[i]);
        }
    }

    // running out of iterations is a sign there isn't enough
    // room for the portal
    return iteration != MAX_POS_ADJUST_ITERATIONS;
}

void portalSurfaceCleanup(struct PortalSurface* portalSurface) {
    if (!portalSurface->shouldCleanup) {
        return;
    }

    gPortalSurfaceCleanupQueue[gPortalSurfaceNextToWrite] = *portalSurface;
    gPortalSurfaceCleanupQueue[gPortalSurfaceNextToWrite].shouldCleanup = 2;
    gPortalSurfaceNextToWrite = (gPortalSurfaceNextToWrite + 1) % MAX_PENDING_PORTAL_CLEANUP;

    portalSurface->shouldCleanup = 0;
}