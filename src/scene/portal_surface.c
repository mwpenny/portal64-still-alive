#include "portal_surface.h"

#include "portal.h"
#include "math/mathf.h"
#include "math/vector2.h"
#include <math.h>
#include "../util/memory.h"

#define NO_OVERLAP  0x10000

#define GET_NEXT_EDGE(surfaceEdge, isReverse) ((isReverse) ? (surfaceEdge)->nextEdgeReverse : (surfaceEdge)->nextEdge)
#define GET_PREV_EDGE(surfaceEdge, isReverse) ((isReverse) ? (surfaceEdge)->prevEdgeReverse : (surfaceEdge)->prevEdge)

#define GET_CURRENT_POINT(surfaceEdge, isReverse) ((isReverse) ? (surfaceEdge)->bIndex : (surfaceEdge->aIndex))
#define GET_NEXT_POINT(surfaceEdge, isReverse) ((isReverse) ? (surfaceEdge)->aIndex : (surfaceEdge->bIndex))

#define SET_NEXT_EDGE(surfaceEdge, isReverse, value) do { if (isReverse) (surfaceEdge)->nextEdgeReverse = (value); else (surfaceEdge)->nextEdge = (value); } while (0)
#define SET_PREV_EDGE(surfaceEdge, isReverse, value) do { if (isReverse) (surfaceEdge)->prevEdgeReverse = (value); else (surfaceEdge)->prevEdge = (value); } while (0)

#define MAX_SEARCH_ITERATIONS   20

#define ADDITIONAL_EDGE_CAPACITY 32
#define ADDITIONAL_VERTEX_CAPACITY 16

struct SurfaceEdgeWithSide {
    int edgeIndex;
    int isReverse;
};

struct PortalSurfaceBuilder {
    struct PortalSurface* surface;
    struct Vector2s16* additionalVertices;
    struct SurfaceEdge* additionalEdges;
    short currentVertex;
    short currentEdge;

    struct SurfaceEdgeWithSide edgeOnSearchLoop;
    union {
        // set when hasEdge is true
        struct SurfaceEdgeWithSide cuttingEdge;
        // set when hasEdge is false
        int startingPoint;
    };
    short hasEdge;
    short hasConnected;
};

int portalSurfaceFindEnclosingFace(struct PortalSurface* surface, struct Vector2s16* aroundPoint, struct SurfaceEdgeWithSide* output) {
    struct SurfaceEdge* currentEdge = &surface->edges[0];
    int edgeDistanceSq = vector2s16DistSqr(&surface->vertices[surface->edges[0].aIndex], aroundPoint);

    for (int i = 1; i < surface->sideCount; ++i) {
        int dist = vector2s16DistSqr(&surface->vertices[surface->edges[i].aIndex], aroundPoint);

        if (dist < edgeDistanceSq) {
            edgeDistanceSq = dist;
            currentEdge = &surface->edges[i];
        }
    }

    int isEdgeReverse = 0;
    int startEdgeIndex = currentEdge - surface->edges;

    int nextEdgeIndex;

    int currentIteration = 0;

    while (currentIteration < MAX_SEARCH_ITERATIONS && (nextEdgeIndex = GET_NEXT_EDGE(currentEdge, isEdgeReverse)) != startEdgeIndex) {
        struct Vector2s16 edgeDir;
        struct Vector2s16 pointDir;

        int anchorPoint = GET_CURRENT_POINT(currentEdge, isEdgeReverse);

        vector2s16Sub(
            &surface->vertices[GET_NEXT_POINT(currentEdge, isEdgeReverse)], 
            &surface->vertices[anchorPoint], 
            &edgeDir
        );

        vector2s16Sub(
            aroundPoint, 
            &surface->vertices[anchorPoint], 
            &pointDir
        );

        if (vector2s16Cross(&edgeDir, &pointDir) < 0) {
            // the point is on the opposite side of this edge
            startEdgeIndex = surface->edges - currentEdge;
            isEdgeReverse = !isEdgeReverse;

            nextEdgeIndex = GET_NEXT_EDGE(currentEdge, isEdgeReverse);

            if (nextEdgeIndex == 0xFF) {
                // has no opposite edge
                return 0;
            }
        }

        int currentIndex = currentEdge - surface->edges;
        currentEdge = &surface->edges[nextEdgeIndex];
        isEdgeReverse = currentEdge->prevEdgeReverse == currentIndex;

        ++currentIteration;
    }

    if (currentIteration == MAX_SEARCH_ITERATIONS) {
        return 0;
    }

    output->edgeIndex = currentEdge - surface->edges;
    output->isReverse = isEdgeReverse;

    return 1;
}

struct SurfaceEdge* portalSurfaceGetEdge(struct PortalSurfaceBuilder* surfaceBuilder, int edgeIndex) {
    if (edgeIndex < surfaceBuilder->surface->edgeCount) {
        return &surfaceBuilder->surface->edges[edgeIndex];
    } else {
        return &surfaceBuilder->additionalEdges[edgeIndex - surfaceBuilder->surface->edgeCount];
    }
}

struct Vector2s16* portalSurfaceGetPoint(struct PortalSurfaceBuilder* surfaceBuilder, int vertexIndex) {
    if (vertexIndex < surfaceBuilder->surface->vertexCount) {
        return &surfaceBuilder->surface->vertices[vertexIndex];
    } else {
        return &surfaceBuilder->additionalVertices[vertexIndex - surfaceBuilder->surface->vertexCount];
    }
}

void portalSurfaceNextEdge(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* currentEdge, struct SurfaceEdgeWithSide* nextEdge) {
    struct SurfaceEdge* edge = portalSurfaceGetEdge(surfaceBuilder, currentEdge->edgeIndex);

    nextEdge->edgeIndex = GET_NEXT_EDGE(edge, currentEdge->isReverse);
    nextEdge->isReverse = portalSurfaceGetEdge(surfaceBuilder, nextEdge->edgeIndex)->prevEdgeReverse == currentEdge->edgeIndex;
}

void portalSurfacePrevEdge(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* currentEdge, struct SurfaceEdgeWithSide* prevEdge) {
    struct SurfaceEdge* edge = portalSurfaceGetEdge(surfaceBuilder, currentEdge->edgeIndex);

    prevEdge->edgeIndex = GET_PREV_EDGE(edge, currentEdge->isReverse);
    prevEdge->isReverse = portalSurfaceGetEdge(surfaceBuilder, prevEdge->edgeIndex)->nextEdgeReverse == currentEdge->edgeIndex;
}

int portalSurfacePointInsideFace(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* currentEdge, struct Vector2s16* point) {
    struct SurfaceEdgeWithSide nextEdge;
    portalSurfaceNextEdge(surfaceBuilder, currentEdge, &nextEdge);

    struct SurfaceEdge* edgePtr = portalSurfaceGetEdge(surfaceBuilder, currentEdge->edgeIndex);
    struct SurfaceEdge* nextEdgePtr = portalSurfaceGetEdge(surfaceBuilder, nextEdge.edgeIndex);

    struct Vector2s16* corner = portalSurfaceGetPoint(surfaceBuilder, GET_NEXT_POINT(edgePtr, currentEdge->isReverse));
    struct Vector2s16* prevPoint = portalSurfaceGetPoint(surfaceBuilder, GET_CURRENT_POINT(edgePtr, currentEdge->isReverse));
    struct Vector2s16* nextPoint = portalSurfaceGetPoint(surfaceBuilder, GET_NEXT_POINT(nextEdgePtr, nextEdge.isReverse));

    struct Vector2s16 nextDir;
    struct Vector2s16 prevDir;

    struct Vector2s16 pointDir;

    vector2s16Sub(nextPoint, corner, &nextDir);
    vector2s16Sub(prevPoint, corner, &prevDir);
    vector2s16Sub(point, corner, &pointDir);

    return vector2s16FallsBetween(&nextDir, &prevDir, &pointDir);
}

int portalSurfaceIsEdgeValid(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* currentEdge) {
    return GET_NEXT_EDGE(portalSurfaceGetEdge(surfaceBuilder, currentEdge->edgeIndex), currentEdge->isReverse) != NO_EDGE_CONNECTION;
}

#define MAX_LOOP_SEARCH_ITERATIONS 20

int portalSurfaceNextLoop(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* currentEdge, struct SurfaceEdgeWithSide* nextFace) {
    portalSurfaceNextEdge(surfaceBuilder, currentEdge, nextFace);
    nextFace->isReverse = !nextFace->isReverse;

    if (portalSurfaceIsEdgeValid(surfaceBuilder, nextFace)) {
        return 1;
    }

    *nextFace = *currentEdge;
    nextFace->isReverse = !nextFace->isReverse;

    int i = 0;

    while (i < MAX_LOOP_SEARCH_ITERATIONS && portalSurfaceIsEdgeValid(surfaceBuilder, nextFace)) {
        struct SurfaceEdgeWithSide prevFace;
        portalSurfacePrevEdge(surfaceBuilder, nextFace, &prevFace);

        *nextFace = prevFace;
        nextFace->isReverse = !nextFace->isReverse;
        ++i;
    }

    nextFace->isReverse = !nextFace->isReverse;

    return i < MAX_LOOP_SEARCH_ITERATIONS;
}

int portalSurfaceFindNextLoop(struct PortalSurfaceBuilder* surfaceBuilder, struct Vector2s16* forPoint) {
    if (!surfaceBuilder->hasConnected) {
        return 1;
    }

    struct SurfaceEdgeWithSide currentEdge = surfaceBuilder->edgeOnSearchLoop;

    int i = 0;

    while (i < MAX_LOOP_SEARCH_ITERATIONS && !portalSurfacePointInsideFace(surfaceBuilder, &currentEdge, forPoint)) {
        struct SurfaceEdgeWithSide nextEdge;

        if (!portalSurfaceNextLoop(surfaceBuilder, &currentEdge, &nextEdge)) {
            return 0;
        }

        if (nextEdge.edgeIndex == surfaceBuilder->edgeOnSearchLoop.edgeIndex) {
            // a full loop and no face found
            return 0;
        }

        currentEdge = nextEdge;
        ++i;
    }

    surfaceBuilder->edgeOnSearchLoop = currentEdge;

    return i < MAX_LOOP_SEARCH_ITERATIONS;
}

#define MAX_INTERSECT_LOOPS 20

int portalSurfaceIsPointOnLine(struct Vector2s16* pointA, struct Vector2s16* edgeA, struct Vector2s16* edgeDir) {
    struct Vector2s16 originOffset;
    vector2s16Sub(edgeA, pointA, &originOffset);

    s64 magProduct = (s64)vector2s16MagSqr(edgeDir) * vector2s16MagSqr(&originOffset);
    s64 dotProduct = vector2s16Dot(edgeDir, &originOffset);

    return magProduct == dotProduct * dotProduct;
}

enum IntersectionType {
    IntersectionTypeNone,
    IntersectionTypePoint,
    IntersectionTypeColinear
};

enum IntersectionType portalSurfaceIntersect(struct Vector2s16* pointA, struct Vector2s16* pointDir, struct Vector2s16* edgeA, struct Vector2s16* edgeB, struct Vector2s16* intersection) {
    struct Vector2s16 edgeDir;
    vector2s16Sub(edgeB, edgeA, &edgeDir);

    struct Vector2s16 originOffset;
    vector2s16Sub(edgeA, pointA, &originOffset);

    int pointLerp = vector2s16Cross(&originOffset, &edgeDir);

    if (pointLerp <= 0) {
        return IntersectionTypeNone;
    }

    int denominator = vector2s16Cross(pointDir, &edgeDir);

    if (denominator == 0) {
        if (!portalSurfaceIsPointOnLine(pointA, edgeA, &edgeDir)) {
            return IntersectionTypeNone;
        }

        int directionDot = vector2s16Dot(pointDir, &edgeDir);

        // find the point furthest in the direction of pointDir
        // that is on both line segments
        if (directionDot > 0) {
            // pointing towards b
            if (vector2s16DistSqr(edgeB, pointA) >= vector2s16MagSqr(pointDir)) {
                // edge ends first
                *intersection = *edgeB;
            } else {
                // point ends first
                vector2s16Add(pointA, pointDir, intersection);
            }
        } else {
            if (vector2s16MagSqr(&originOffset) >= vector2s16MagSqr(pointDir)) {
                // edge ends first
                *intersection = *edgeA;
            } else {
                // point ends first
                vector2s16Add(pointA, pointDir, intersection);
            }
        }
        
        // the lines are colinear
        return IntersectionTypeColinear;
    }

    if (pointLerp > denominator || denominator == 0) {
        return IntersectionTypeNone;
    }

    int edgeLerp = vector2s16Cross(pointDir, &originOffset);

    if (edgeLerp < 0 || edgeLerp > denominator) {
        return IntersectionTypeNone;
    }

    intersection->x = (short)((s64)pointDir->x * (s64)pointLerp / (s64)denominator) + pointA->x;
    intersection->y = (short)((s64)pointDir->y * (s64)pointLerp / (s64)denominator) + pointA->y;

    return IntersectionTypePoint;
}

int portalSurfaceNewVertex(struct PortalSurfaceBuilder* surfaceBuilder, struct Vector2s16* point) {
    if (surfaceBuilder->currentVertex == ADDITIONAL_VERTEX_CAPACITY) {
        return -1;
    }

    surfaceBuilder->additionalVertices[surfaceBuilder->currentVertex] = *point;
    int newVertexIndex = surfaceBuilder->currentVertex + surfaceBuilder->surface->vertexCount;
    ++surfaceBuilder->currentVertex;

    return newVertexIndex;
}

int portalSurfaceNewEdge(struct PortalSurfaceBuilder* surfaceBuilder) {
    if (surfaceBuilder->currentEdge == ADDITIONAL_EDGE_CAPACITY) {
        return -1;
    }

    int newEdgeIndex = surfaceBuilder->currentEdge + surfaceBuilder->surface->edgeCount;
    ++surfaceBuilder->currentEdge;
    return newEdgeIndex;
}

int portalSurfaceSplitEdge(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* edge, struct Vector2s16* point) {
    int newVertexIndex = portalSurfaceNewVertex(surfaceBuilder, point);
    
    if (newVertexIndex == -1) {
        return -1;
    }
    
    struct SurfaceEdge* existingEdge = portalSurfaceGetEdge(surfaceBuilder, edge->edgeIndex);

    struct SurfaceEdgeWithSide nextEdge;
    struct SurfaceEdgeWithSide prevReverseEdge;

    portalSurfaceNextEdge(surfaceBuilder, edge, &nextEdge);
    portalSurfacePrevEdge(surfaceBuilder, edge, &prevReverseEdge);

    int newEdgeIndex = portalSurfaceNewEdge(surfaceBuilder);

    if (newEdgeIndex == -1) {
        return -1;
    }

    struct SurfaceEdge* newEdge = portalSurfaceGetEdge(surfaceBuilder, newEdgeIndex);

    newEdge->bIndex = existingEdge->bIndex;
    newEdge->aIndex = newVertexIndex;
    newEdge->nextEdge = existingEdge->nextEdge;
    newEdge->prevEdge = edge->edgeIndex;
    newEdge->nextEdgeReverse = edge->edgeIndex;
    newEdge->prevEdgeReverse = existingEdge->prevEdgeReverse;
    
    existingEdge->bIndex = newVertexIndex;
    existingEdge->nextEdge = newEdgeIndex;
    existingEdge->prevEdgeReverse = newVertexIndex;

    struct SurfaceEdge* nextEdgePtr = portalSurfaceGetEdge(surfaceBuilder, nextEdge.edgeIndex);

    if (nextEdge.isReverse) {
        nextEdgePtr->prevEdgeReverse = newEdgeIndex;
    } else {
        nextEdgePtr->prevEdge = newEdgeIndex;
    }

    struct SurfaceEdge* prevEdgePtr = portalSurfaceGetEdge(surfaceBuilder, prevReverseEdge.edgeIndex);

    SET_NEXT_EDGE(prevEdgePtr, prevReverseEdge.isReverse, newEdgeIndex);

    return newVertexIndex;
}

int portalSurfaceConnectToPoint(struct PortalSurfaceBuilder* surfaceBuilder, int pointIndex, struct SurfaceEdgeWithSide* edge) {
    int newEdge = portalSurfaceNewEdge(surfaceBuilder);

    if (newEdge == -1) {
        return 0;
    }

    struct SurfaceEdge* newEdgePtr = portalSurfaceGetEdge(surfaceBuilder, newEdge);

    if (surfaceBuilder->hasEdge) {
        struct SurfaceEdgeWithSide nextEdge;
        portalSurfaceNextEdge(surfaceBuilder, &surfaceBuilder->cuttingEdge, &nextEdge);

        struct SurfaceEdge* cuttingEdgePtr = portalSurfaceGetEdge(surfaceBuilder, surfaceBuilder->cuttingEdge.edgeIndex);
        struct SurfaceEdge* nextEdgePtr = portalSurfaceGetEdge(surfaceBuilder, nextEdge.edgeIndex);

        newEdgePtr->aIndex = GET_NEXT_POINT(cuttingEdgePtr, surfaceBuilder->cuttingEdge.isReverse);

        newEdgePtr->prevEdge = surfaceBuilder->cuttingEdge.edgeIndex;
        newEdgePtr->nextEdgeReverse = nextEdge.edgeIndex;

        SET_NEXT_EDGE(cuttingEdgePtr, surfaceBuilder->cuttingEdge.isReverse, newEdge);
        SET_PREV_EDGE(nextEdgePtr, nextEdge.isReverse, newEdge);
    } else {
        newEdgePtr->prevEdge = newEdge;
        newEdgePtr->nextEdgeReverse = newEdge;
        newEdgePtr->aIndex = surfaceBuilder->startingPoint;
    }

    newEdgePtr->bIndex = pointIndex;

    if (edge) {
        struct SurfaceEdgeWithSide nextEdge;
        portalSurfaceNextEdge(surfaceBuilder, edge, &nextEdge);

        struct SurfaceEdge* edgePtr = portalSurfaceGetEdge(surfaceBuilder, edge->edgeIndex);
        struct SurfaceEdge* nextEdgePtr = portalSurfaceGetEdge(surfaceBuilder, nextEdge.edgeIndex);

        newEdgePtr->nextEdge = nextEdge.edgeIndex;
        newEdgePtr->prevEdgeReverse = edge->edgeIndex;

        SET_NEXT_EDGE(edgePtr, edge->isReverse, newEdge);
        SET_PREV_EDGE(nextEdgePtr, nextEdge.isReverse, newEdge);
    } else {
        newEdgePtr->nextEdge = newEdge;
        newEdgePtr->prevEdgeReverse = newEdge;
    }

    surfaceBuilder->hasEdge = 1;
    surfaceBuilder->cuttingEdge.edgeIndex = newEdge;
    surfaceBuilder->cuttingEdge.isReverse = 0;

    return 1;
}

struct Vector2s16* portalSurfaceIntersectEdgeWithLoop(struct PortalSurfaceBuilder* surfaceBuilder, struct Vector2s16* pointA, struct Vector2s16* pointDir) {
    struct SurfaceEdgeWithSide currentEdge = surfaceBuilder->edgeOnSearchLoop;

    for (int iteration = 0; iteration < MAX_INTERSECT_LOOPS; ++iteration) {
        struct SurfaceEdgeWithSide nextEdge;

        portalSurfaceNextEdge(surfaceBuilder, &currentEdge, &nextEdge);

        if (nextEdge.edgeIndex == surfaceBuilder->edgeOnSearchLoop.edgeIndex && nextEdge.isReverse == surfaceBuilder->edgeOnSearchLoop.isReverse) {
            // finished searching loop
            break;
        }

        struct SurfaceEdge* edge = portalSurfaceGetEdge(surfaceBuilder, currentEdge.edgeIndex);

        struct Vector2s16* edgeA = portalSurfaceGetPoint(surfaceBuilder, GET_CURRENT_POINT(edge, currentEdge.isReverse));
        struct Vector2s16* edgeB = portalSurfaceGetPoint(surfaceBuilder, GET_NEXT_POINT(edge, currentEdge.isReverse));

        struct Vector2s16 intersectionPoint;

        enum IntersectionType intersectType = portalSurfaceIntersect(pointA, pointDir, edgeA, edgeB, &intersectionPoint);

        if (intersectionPoint.equalTest == pointA->equalTest) {
            currentEdge = nextEdge;
            continue;
        }

        if (intersectType == IntersectionTypePoint) {
            int newPointIndex;

            if (intersectionPoint.equalTest == edgeA->equalTest) {
                newPointIndex = GET_CURRENT_POINT(edge, currentEdge.isReverse);
            } else if (intersectionPoint.equalTest == edgeB->equalTest) {
                newPointIndex = GET_NEXT_POINT(edge, currentEdge.isReverse);
            } else {
                newPointIndex = portalSurfaceSplitEdge(surfaceBuilder, &currentEdge, &intersectionPoint);
                
                if (newPointIndex == -1) {
                    return NULL;
                }
            }

            if (!portalSurfaceConnectToPoint(surfaceBuilder, newPointIndex, &currentEdge)) {
                return NULL;
            }

            surfaceBuilder->hasConnected = 1;

            return portalSurfaceGetPoint(surfaceBuilder, newPointIndex);
        } else if (intersectType == IntersectionTypeColinear) {
            int newPointIndex;

            if (intersectionPoint.equalTest == edgeA->equalTest) {
                newPointIndex = GET_CURRENT_POINT(edge, currentEdge.isReverse);
                portalSurfacePrevEdge(surfaceBuilder, &currentEdge, &surfaceBuilder->cuttingEdge);
            } else if (intersectionPoint.equalTest == edgeB->equalTest) {
                newPointIndex = GET_NEXT_POINT(edge, currentEdge.isReverse);
                surfaceBuilder->cuttingEdge = currentEdge;
            } else {
                int newPointIndex = portalSurfaceSplitEdge(surfaceBuilder, &currentEdge, &intersectionPoint);

                if (newPointIndex == -1) {
                    return NULL;
                }

                surfaceBuilder->cuttingEdge = currentEdge;
            }

            surfaceBuilder->hasEdge = 1;
            surfaceBuilder->hasConnected = 1;

            return portalSurfaceGetPoint(surfaceBuilder, newPointIndex);
        }

        currentEdge = nextEdge;
    }

    struct Vector2s16 pointB;
    vector2s16Add(pointA, pointDir, &pointB);

    int newPointIndex = portalSurfaceNewVertex(surfaceBuilder, &pointB);

    if (!portalSurfaceConnectToPoint(surfaceBuilder, newPointIndex, NULL)) {
        return NULL;
    }

    return portalSurfaceGetPoint(surfaceBuilder, newPointIndex);
}

int portalSurfaceFindStartingPoint(struct PortalSurfaceBuilder* surfaceBuilder, struct Vector2s16* point) {
    struct SurfaceEdgeWithSide currentEdge = surfaceBuilder->edgeOnSearchLoop;

    struct Vector2s16* edgeA = portalSurfaceGetPoint(surfaceBuilder, GET_CURRENT_POINT(portalSurfaceGetEdge(surfaceBuilder, currentEdge.edgeIndex), currentEdge.isReverse));

    for (int iteration = 0; iteration < MAX_INTERSECT_LOOPS; ++iteration) {
        struct SurfaceEdgeWithSide nextEdge;

        portalSurfaceNextEdge(surfaceBuilder, &currentEdge, &nextEdge);

        if (nextEdge.edgeIndex == surfaceBuilder->edgeOnSearchLoop.edgeIndex && nextEdge.isReverse == surfaceBuilder->edgeOnSearchLoop.isReverse) {
            // finished searching loop
            break;
        }

        struct Vector2s16* edgeB = portalSurfaceGetPoint(surfaceBuilder, GET_NEXT_POINT(portalSurfaceGetEdge(surfaceBuilder, currentEdge.edgeIndex), currentEdge.isReverse));

        struct Vector2s16 edgeDir;
        vector2s16Sub(edgeB, edgeA, &edgeDir);

        if (portalSurfaceIsPointOnLine(point, edgeA, &edgeDir)) {
            surfaceBuilder->hasEdge = 1;
            surfaceBuilder->hasConnected = 1;

            if (point->equalTest == edgeA->equalTest) {
                portalSurfacePrevEdge(surfaceBuilder, &currentEdge, &surfaceBuilder->cuttingEdge);
            } else if (point->equalTest == edgeB->equalTest) {
                surfaceBuilder->cuttingEdge = currentEdge;
            } else {
                if (portalSurfaceSplitEdge(surfaceBuilder, &currentEdge, point) == -1) {
                    return 0;
                }

                surfaceBuilder->cuttingEdge = currentEdge;
            }

            surfaceBuilder->edgeOnSearchLoop = surfaceBuilder->cuttingEdge;

            return 1;
        }

        edgeA = edgeB;
        currentEdge = nextEdge;
    }

    surfaceBuilder->hasEdge = 0;
    surfaceBuilder->hasConnected = 0;
    surfaceBuilder->startingPoint = surfaceBuilder->surface->vertexCount;
    surfaceBuilder->additionalVertices[0] = *point;
    ++surfaceBuilder->currentVertex;

    return 1;
}

struct PortalSurface* portalSurfaceCutHole(struct PortalSurface* surface, struct Vector2s16* loop, int loopSize) {
    struct PortalSurfaceBuilder surfaceBuilder;

    surfaceBuilder.additionalVertices = stackMalloc(sizeof(struct Vector2s16) * ADDITIONAL_VERTEX_CAPACITY);
    surfaceBuilder.currentVertex = 0;
    surfaceBuilder.additionalEdges = stackMalloc(sizeof(struct SurfaceEdge) * ADDITIONAL_EDGE_CAPACITY);
    surfaceBuilder.currentEdge = 0;

    struct Vector2s16* prev = &loop[0];

    if (!portalSurfaceFindEnclosingFace(surface, prev, &surfaceBuilder.edgeOnSearchLoop)) {
        goto error;
    }

    if (!portalSurfaceFindStartingPoint(&surfaceBuilder, prev)) {
        goto error;
    }

    for (int i = 1; i < loopSize;) {
        struct Vector2s16* next = &loop[i];
        struct Vector2s16 dir;

        if (!portalSurfaceFindNextLoop(&surfaceBuilder, next)) {
            goto error;
        }

        vector2s16Sub(next, prev, &dir);
        struct Vector2s16* newPoint = portalSurfaceIntersectEdgeWithLoop(&surfaceBuilder, prev, &dir);

        if (!newPoint) {
            goto error;
        }

        if (newPoint->equalTest == next->equalTest) {
            // only increment i if the next point was reached
            ++i;
        }

        prev = newPoint;
    }

    stackMallocFree(surfaceBuilder.additionalEdges);
    stackMallocFree(surfaceBuilder.additionalVertices);

    return NULL;

error:
    stackMallocFree(surfaceBuilder.additionalEdges);
    stackMallocFree(surfaceBuilder.additionalVertices);
    return NULL;
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

int portalSurfaceAdjustPosition(struct PortalSurface* surface, struct Transform* portalAt, struct Vector2s16* output, struct Vector2s16* outlineLoopOutput) {
    struct Vector2s16 minPortal;
    struct Vector2s16 maxPortal;

    portalSurface2DPoint(surface, &portalAt->position, output);

    minPortal = *output;
    maxPortal = *output;

    for (int i = 0; i < PORTAL_LOOP_SIZE; ++i) {
        struct Vector2s16 cornerPoint;
        struct Vector3 transformedPoint;
        transformPoint(portalAt, &gPortalOutlineUnscaled[i], &transformedPoint);
        portalSurface2DPoint(surface, &transformedPoint, &cornerPoint);

        minPortal.x = MIN(minPortal.x, cornerPoint.x);
        minPortal.y = MIN(minPortal.y, cornerPoint.y);

        maxPortal.x = MAX(maxPortal.x, cornerPoint.x);
        maxPortal.y = MAX(maxPortal.y, cornerPoint.y);
    }


    struct Vector2s16 halfSize;
    halfSize.x = (maxPortal.x - minPortal.x) >> 1;
    halfSize.y = (maxPortal.y - minPortal.y) >> 1;

    int iteration = 0;

    for (iteration = 0; iteration < MAX_POS_ADJUST_ITERATIONS; ++iteration) {
        int minOverlap = NO_OVERLAP;
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

        if (minOverlap == NO_OVERLAP) {
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

    portalSurfaceInverse(surface, &correctPosition, &portalAt->position);
    // TODO
    // loop through all triangles
    // if no interesctions on triangle pass it through
    // if triangle intersects portal then retrianglute face

    return 1;
}