#include "portal_surface.h"

#include "portal.h"
#include "math/mathf.h"
#include "math/vector2.h"
#include <math.h>
#include "portal_surface_generator.h"
#include "../util/memory.h"

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

int portalSurfaceAdjustPosition(struct PortalSurface* surface, struct Transform* portalAt, struct Vector2s16* output, struct Vector2s16* outlineLoopOutput) {
    struct Vector2s16 minPortal;
    struct Vector2s16 maxPortal;

    portalSurface2DPoint(surface, &portalAt->position, output);

    struct Vector3 portalNormal;
    quatMultVector(&portalAt->rotation, &gForward, &portalNormal);

    struct Vector3 surfaceNormal;
    vector3Cross(&surface->right, &surface->up, &surfaceNormal);

    int shouldReverse = vector3Dot(&portalNormal, &surfaceNormal) > 0.0f;

    minPortal = *output;
    maxPortal = *output;

    for (int i = 0; i < PORTAL_LOOP_SIZE; ++i) {
        struct Vector3 transformedPoint;
        transformPoint(portalAt, &gPortalOutlineUnscaled[shouldReverse ? (PORTAL_LOOP_SIZE - 1) - i : i], &transformedPoint);
        portalSurface2DPoint(surface, &transformedPoint, &outlineLoopOutput[i]);

        minPortal.x = MIN(minPortal.x, outlineLoopOutput[i].x);
        minPortal.y = MIN(minPortal.y, outlineLoopOutput[i].y);

        maxPortal.x = MAX(maxPortal.x, outlineLoopOutput[i].x);
        maxPortal.y = MAX(maxPortal.y, outlineLoopOutput[i].y);
    }


    struct Vector2s16 halfSize;
    halfSize.x = (maxPortal.x - minPortal.x) >> 1;
    halfSize.y = (maxPortal.y - minPortal.y) >> 1;

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

    // running out of iterations is a sign there isn't enough
    // room for the portal
    return iteration != MAX_POS_ADJUST_ITERATIONS;
}

int portalSurfaceGenerate(struct PortalSurface* surface, struct Transform* portalAt, Vtx* vertices, Gfx* triangles) {
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

    struct PortalSurface* newSurface = newPortalSurfaceWithHole(surface, portalOutline);

    if (newSurface) {
        deletePortalSurface(newSurface);
    }

    portalSurfaceInverse(surface, &correctPosition, &portalAt->position);
    // TODO
    // loop through all triangles
    // if no interesctions on triangle pass it through
    // if triangle intersects portal then retrianglute face

    return 1;
}

void deletePortalSurface(struct PortalSurface* portalSurface) {
    free(portalSurface->vertices);
    free(portalSurface->edges);
    free(portalSurface->gfxVertices);
    free(portalSurface->triangles);
    free(portalSurface);
}