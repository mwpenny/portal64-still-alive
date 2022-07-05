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

struct PortalSurfaceReplacement gPortalSurfaceReplacements[2];

void portalSurfaceReplacementRevert(struct PortalSurfaceReplacement* replacement) {
    gCurrentLevel->staticContent[replacement->staticIndex].displayList = replacement->previousSurface.triangles;
    portalSurfaceCleanup(&gCurrentLevel->portalSurfaces[replacement->portalSurfaceIndex]);
    gCurrentLevel->portalSurfaces[replacement->portalSurfaceIndex] = replacement->previousSurface;
    replacement->flags = 0;
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

void portalSurfaceReplace(int portalSurfaceIndex, int roomIndex, int portalIndex, struct PortalSurface* with) {
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
        return;
    }

    replacement->previousSurface = *existing;
    replacement->flags = PortalSurfaceReplacementFlagsIsEnabled;
    replacement->staticIndex = staticIndex;
    replacement->portalSurfaceIndex = portalSurfaceIndex;

    gCurrentLevel->staticContent[staticIndex].displayList = with->triangles;
    gCurrentLevel->portalSurfaces[portalSurfaceIndex] = *with;
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

    struct Vector2s16 portalPosition;
    portalSurface2DPoint(surface, &portalAt->position, &portalPosition);

    for (int i = 0; i < surface->sideCount; ++i) {
        struct SurfaceEdge* edge = &surface->edges[i];
        struct Vector2s16 a = surface->vertices[edge->aIndex];
        struct Vector2s16 b = surface->vertices[edge->bIndex];

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

        for (int i = 0; i < surface->sideCount; ++i) {
            struct SurfaceEdge* edge = &surface->edges[i];
            struct Vector2s16 a = surface->vertices[edge->aIndex];

            int offsetX = output->x - a.x;
            int offsetY = output->y - a.y;

            int xOverlap = halfSize.x - abs(offsetX);
            int yOverlap = halfSize.y - abs(offsetY);

            // check if vertex is inside portal BB
            if (xOverlap > 0 && yOverlap > 0) {
                if (xOverlap < yOverlap) {
                    if (xOverlap < minOverlap) {
                        minOverlap = xOverlap;
                        minOverlapOffset.x = xOverlap * sign(offsetX);
                        minOverlapOffset.y = 0;
                    }
                } else {
                    if (yOverlap < minOverlap) {
                        minOverlap = yOverlap;
                        minOverlapOffset.x = 0;
                        minOverlapOffset.y = yOverlap * sign(offsetY);
                    }
                }

                continue;
            }

            // check if line intersects portal BB

            struct Vector2s16 b = surface->vertices[edge->bIndex];

            struct Vector2s16 offset;
            offset.x = b.x - a.x;
            offset.y = b.y - a.y;

            int lineAxis;
            int crossAxis;
            int crossDirection;

            if (abs(offset.x) > abs(offset.y)) {
                lineAxis = 0;
                crossAxis = 1;
                crossDirection = -sign(offset.x);
            } else {
                lineAxis = 1;
                crossAxis = 0;
                crossDirection = sign(offset.y);
            }

            int portalPosLineAxis = VECTOR2s16_AS_ARRAY(output)[lineAxis];

            // check line endpoints relative to the portal position
            if ((VECTOR2s16_AS_ARRAY(&a)[lineAxis] - portalPosLineAxis) * (VECTOR2s16_AS_ARRAY(&b)[lineAxis] - portalPosLineAxis) >= 0) {
                continue;
            }

            int boxPosition = VECTOR2s16_AS_ARRAY(output)[crossAxis] + crossDirection * VECTOR2s16_AS_ARRAY(&halfSize)[crossAxis];
            int distance = (boxPosition - VECTOR2s16_AS_ARRAY(&a)[crossAxis]) * crossDirection;

            if (distance <= 0 || distance >= VECTOR2s16_AS_ARRAY(&halfSize)[crossAxis]) {
                continue;
            }

            if (distance < minOverlap) {
                minOverlap = distance;
                VECTOR2s16_AS_ARRAY(&minOverlapOffset)[lineAxis] = 0;
                VECTOR2s16_AS_ARRAY(&minOverlapOffset)[crossAxis] = -distance * crossDirection;
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

int portalSurfaceGenerate(struct PortalSurface* surface, struct Transform* portalAt, struct PortalSurface* newSurface) {
    // determine if portal is on surface
    if (!portalSurfaceIsInside(surface, portalAt)) {
        return 0;
    }
    // find all portal edge intersections
    struct Vector2s16 correctPosition;
    struct Vector2s16 portalOutline[PORTAL_LOOP_SIZE];
    if (!portalSurfaceAdjustPosition(surface, portalAt, &correctPosition, portalOutline)) {
        return 0;
    }

    if (!portalSurfacePokeHole(surface, portalOutline, newSurface)) {
        return 0;
    }

    portalSurfaceInverse(surface, &correctPosition, &portalAt->position);

    return 1;
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