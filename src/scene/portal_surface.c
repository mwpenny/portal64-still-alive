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

#define ADDITIONAL_EDGE_CAPACITY 64
#define ADDITIONAL_VERTEX_CAPACITY 32

struct SurfaceEdgeWithSide {
    int edgeIndex;
    int isReverse;
};

struct PortalSurfaceBuilder {
    struct Vector2s16* vertices;
    struct SurfaceEdge* edges;
    short vertexCount;
    short edgeCount;

    u8* isLoopEdge;
    u8* isEdgeUsed;

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
    struct SurfaceEdge* edgeCount = &surface->edges[0];
    int edgeDistanceSq = vector2s16DistSqr(&surface->vertices[surface->edges[0].aIndex], aroundPoint);

    for (int i = 1; i < surface->sideCount; ++i) {
        int dist = vector2s16DistSqr(&surface->vertices[surface->edges[i].aIndex], aroundPoint);

        if (dist < edgeDistanceSq) {
            edgeDistanceSq = dist;
            edgeCount = &surface->edges[i];
        }
    }

    int isEdgeReverse = 0;
    int startEdgeIndex = edgeCount - surface->edges;

    int currentIteration = 0;

    while (currentIteration < MAX_SEARCH_ITERATIONS && (currentIteration == 0 || (edgeCount - surface->edges) != startEdgeIndex)) {
        struct Vector2s16 edgeDir;
        struct Vector2s16 pointDir;

        int anchorPoint = GET_CURRENT_POINT(edgeCount, isEdgeReverse);

        vector2s16Sub(
            &surface->vertices[GET_NEXT_POINT(edgeCount, isEdgeReverse)], 
            &surface->vertices[anchorPoint], 
            &edgeDir
        );

        vector2s16Sub(
            aroundPoint, 
            &surface->vertices[anchorPoint], 
            &pointDir
        );

        int nextEdgeIndex;

        if (vector2s16Cross(&edgeDir, &pointDir) < 0) {
            // the point is on the opposite side of this edge
            startEdgeIndex = edgeCount - surface->edges;
            isEdgeReverse = !isEdgeReverse;

            nextEdgeIndex = GET_NEXT_EDGE(edgeCount, isEdgeReverse);

            if (nextEdgeIndex == 0xFF) {
                // has no opposite edge
                return 0;
            }
        } else {
            nextEdgeIndex = GET_NEXT_EDGE(edgeCount, isEdgeReverse);
        }

        int currentIndex = edgeCount - surface->edges;
        edgeCount = &surface->edges[nextEdgeIndex];
        isEdgeReverse = edgeCount->prevEdgeReverse == currentIndex;

        ++currentIteration;
    }

    if (currentIteration == MAX_SEARCH_ITERATIONS) {
        return 0;
    }

    output->edgeIndex = edgeCount - surface->edges;
    output->isReverse = isEdgeReverse;

    return 1;
}

struct SurfaceEdge* portalSurfaceGetEdge(struct PortalSurfaceBuilder* surfaceBuilder, int edgeIndex) {
    return &surfaceBuilder->edges[edgeIndex];
}

struct Vector2s16* portalSurfaceGetVertex(struct PortalSurfaceBuilder* surfaceBuilder, int vertexIndex) {
    return &surfaceBuilder->vertices[vertexIndex];
}

void portalSurfaceNextEdge(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* edgeCount, struct SurfaceEdgeWithSide* nextEdge) {
    struct SurfaceEdge* edge = portalSurfaceGetEdge(surfaceBuilder, edgeCount->edgeIndex);
    int edgeIndex = edgeCount->edgeIndex;

    nextEdge->edgeIndex = GET_NEXT_EDGE(edge, edgeCount->isReverse);
    nextEdge->isReverse = portalSurfaceGetEdge(surfaceBuilder, nextEdge->edgeIndex)->prevEdgeReverse == edgeIndex;
}

void portalSurfacePrevEdge(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* edgeCount, struct SurfaceEdgeWithSide* prevEdge) {
    struct SurfaceEdge* edge = portalSurfaceGetEdge(surfaceBuilder, edgeCount->edgeIndex);
    int edgeIndex = edgeCount->edgeIndex;

    prevEdge->edgeIndex = GET_PREV_EDGE(edge, edgeCount->isReverse);
    prevEdge->isReverse = portalSurfaceGetEdge(surfaceBuilder, prevEdge->edgeIndex)->nextEdgeReverse == edgeIndex;
}

int portalSurfacePointInsideFace(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* edgeCount, struct Vector2s16* point) {
    struct SurfaceEdgeWithSide nextEdge;
    portalSurfaceNextEdge(surfaceBuilder, edgeCount, &nextEdge);

    struct SurfaceEdge* edgePtr = portalSurfaceGetEdge(surfaceBuilder, edgeCount->edgeIndex);
    struct SurfaceEdge* nextEdgePtr = portalSurfaceGetEdge(surfaceBuilder, nextEdge.edgeIndex);

    struct Vector2s16* corner = portalSurfaceGetVertex(surfaceBuilder, GET_NEXT_POINT(edgePtr, edgeCount->isReverse));
    struct Vector2s16* prevPoint = portalSurfaceGetVertex(surfaceBuilder, GET_CURRENT_POINT(edgePtr, edgeCount->isReverse));
    struct Vector2s16* nextPoint = portalSurfaceGetVertex(surfaceBuilder, GET_NEXT_POINT(nextEdgePtr, nextEdge.isReverse));

    struct Vector2s16 nextDir;
    struct Vector2s16 prevDir;

    struct Vector2s16 pointDir;

    vector2s16Sub(nextPoint, corner, &nextDir);
    vector2s16Sub(prevPoint, corner, &prevDir);
    vector2s16Sub(point, corner, &pointDir);

    return vector2s16FallsBetween(&nextDir, &prevDir, &pointDir);
}

int portalSurfaceIsEdgeValid(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* edgeCount) {
    return GET_NEXT_EDGE(portalSurfaceGetEdge(surfaceBuilder, edgeCount->edgeIndex), edgeCount->isReverse) != NO_EDGE_CONNECTION;
}

#define MAX_LOOP_SEARCH_ITERATIONS 20

int portalSurfaceNextLoop(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* edgeCount, struct SurfaceEdgeWithSide* nextFace) {
    portalSurfaceNextEdge(surfaceBuilder, edgeCount, nextFace);
    nextFace->isReverse = !nextFace->isReverse;

    if (portalSurfaceIsEdgeValid(surfaceBuilder, nextFace)) {
        return 1;
    }

    *nextFace = *edgeCount;
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

    struct SurfaceEdgeWithSide edgeCount = surfaceBuilder->edgeOnSearchLoop;

    int i = 0;

    while (i < MAX_LOOP_SEARCH_ITERATIONS && !portalSurfacePointInsideFace(surfaceBuilder, &edgeCount, forPoint)) {
        struct SurfaceEdgeWithSide nextEdge;

        if (!portalSurfaceNextLoop(surfaceBuilder, &edgeCount, &nextEdge)) {
            return 0;
        }

        if (i != 0 && edgeCount.edgeIndex == surfaceBuilder->edgeOnSearchLoop.edgeIndex) {
            // a full loop and no face found
            return 0;
        }

        edgeCount = nextEdge;
        ++i;
    }

    surfaceBuilder->edgeOnSearchLoop = edgeCount;

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

    int denominator = vector2s16Cross(&edgeDir, pointDir);

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

    int pointLerp = vector2s16Cross(&edgeDir, &originOffset);
    
    if (denominator > 0 ? (pointLerp <= 0 || pointLerp > denominator) : (pointLerp >= 0 || pointLerp < denominator)) {
        return IntersectionTypeNone;
    }

    int edgeLerp = vector2s16Cross(pointDir, &originOffset);

    if (denominator > 0 ? (edgeLerp < 0 || edgeLerp > denominator) : (edgeLerp > 0 || edgeLerp < denominator)) {
        return IntersectionTypeNone;
    }

    intersection->x = (short)((s64)pointDir->x * (s64)pointLerp / (s64)denominator) + pointA->x;
    intersection->y = (short)((s64)pointDir->y * (s64)pointLerp / (s64)denominator) + pointA->y;

    return IntersectionTypePoint;
}

int portalSurfaceNewVertex(struct PortalSurfaceBuilder* surfaceBuilder, struct Vector2s16* point) {
    if (surfaceBuilder->vertexCount == ADDITIONAL_VERTEX_CAPACITY) {
        return -1;
    }

    int newVertexIndex = surfaceBuilder->vertexCount;
    surfaceBuilder->vertices[newVertexIndex] = *point;
    ++surfaceBuilder->vertexCount;

    return newVertexIndex;
}

int portalSurfaceNewEdge(struct PortalSurfaceBuilder* surfaceBuilder, int isLoopEdge) {
    if (surfaceBuilder->edgeCount == ADDITIONAL_EDGE_CAPACITY) {
        return -1;
    }

    int newEdgeIndex = surfaceBuilder->edgeCount;
    surfaceBuilder->isLoopEdge[surfaceBuilder->edgeCount] = isLoopEdge;
    ++surfaceBuilder->edgeCount;
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

    int newEdgeIndex = portalSurfaceNewEdge(surfaceBuilder, 0);

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
    int newEdge = portalSurfaceNewEdge(surfaceBuilder, 1);

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
    struct SurfaceEdgeWithSide edgeCount = surfaceBuilder->edgeOnSearchLoop;

    int iteration;

    for (iteration = 0; 
        iteration < MAX_INTERSECT_LOOPS && (
            iteration == 0 || 
            edgeCount.edgeIndex != surfaceBuilder->edgeOnSearchLoop.edgeIndex || 
            edgeCount.isReverse != surfaceBuilder->edgeOnSearchLoop.isReverse
        ); portalSurfaceNextEdge(surfaceBuilder, &edgeCount, &edgeCount), ++iteration) {
        struct SurfaceEdge* edge = portalSurfaceGetEdge(surfaceBuilder, edgeCount.edgeIndex);

        struct Vector2s16* edgeA = portalSurfaceGetVertex(surfaceBuilder, GET_CURRENT_POINT(edge, edgeCount.isReverse));
        struct Vector2s16* edgeB = portalSurfaceGetVertex(surfaceBuilder, GET_NEXT_POINT(edge, edgeCount.isReverse));

        struct Vector2s16 intersectionPoint;

        enum IntersectionType intersectType = portalSurfaceIntersect(pointA, pointDir, edgeA, edgeB, &intersectionPoint);

        if (intersectionPoint.equalTest == pointA->equalTest) {
            continue;
        }

        if (intersectType == IntersectionTypePoint) {
            int newPointIndex;

            if (intersectionPoint.equalTest == edgeA->equalTest) {
                newPointIndex = GET_CURRENT_POINT(edge, edgeCount.isReverse);
            } else if (intersectionPoint.equalTest == edgeB->equalTest) {
                newPointIndex = GET_NEXT_POINT(edge, edgeCount.isReverse);
            } else {
                newPointIndex = portalSurfaceSplitEdge(surfaceBuilder, &edgeCount, &intersectionPoint);
                
                if (newPointIndex == -1) {
                    return NULL;
                }
            }

            if (!portalSurfaceConnectToPoint(surfaceBuilder, newPointIndex, &edgeCount)) {
                return NULL;
            }

            surfaceBuilder->hasConnected = 1;

            return portalSurfaceGetVertex(surfaceBuilder, newPointIndex);
        } else if (intersectType == IntersectionTypeColinear) {
            int newPointIndex;

            if (intersectionPoint.equalTest == edgeA->equalTest) {
                newPointIndex = GET_CURRENT_POINT(edge, edgeCount.isReverse);
                portalSurfacePrevEdge(surfaceBuilder, &edgeCount, &surfaceBuilder->cuttingEdge);
            } else if (intersectionPoint.equalTest == edgeB->equalTest) {
                newPointIndex = GET_NEXT_POINT(edge, edgeCount.isReverse);
                surfaceBuilder->cuttingEdge = edgeCount;
            } else {
                newPointIndex = portalSurfaceSplitEdge(surfaceBuilder, &edgeCount, &intersectionPoint);

                if (newPointIndex == -1) {
                    return NULL;
                }

                surfaceBuilder->cuttingEdge = edgeCount;
            }

            surfaceBuilder->hasEdge = 1;
            surfaceBuilder->hasConnected = 1;

            return portalSurfaceGetVertex(surfaceBuilder, newPointIndex);
        }
    }

    struct Vector2s16 pointB;
    vector2s16Add(pointA, pointDir, &pointB);

    int newPointIndex = portalSurfaceNewVertex(surfaceBuilder, &pointB);

    if (!portalSurfaceConnectToPoint(surfaceBuilder, newPointIndex, NULL)) {
        return NULL;
    }

    return portalSurfaceGetVertex(surfaceBuilder, newPointIndex);
}

int portalSurfaceFindStartingPoint(struct PortalSurfaceBuilder* surfaceBuilder, struct Vector2s16* point) {
    struct SurfaceEdgeWithSide edgeCount = surfaceBuilder->edgeOnSearchLoop;

    struct Vector2s16* edgeA = portalSurfaceGetVertex(surfaceBuilder, GET_CURRENT_POINT(portalSurfaceGetEdge(surfaceBuilder, edgeCount.edgeIndex), edgeCount.isReverse));

    for (int iteration = 0; iteration < MAX_INTERSECT_LOOPS; ++iteration) {
        if (iteration > 0 && edgeCount.edgeIndex == surfaceBuilder->edgeOnSearchLoop.edgeIndex && edgeCount.isReverse == surfaceBuilder->edgeOnSearchLoop.isReverse) {
            // finished searching loop
            break;
        }

        struct Vector2s16* edgeB = portalSurfaceGetVertex(surfaceBuilder, GET_NEXT_POINT(portalSurfaceGetEdge(surfaceBuilder, edgeCount.edgeIndex), edgeCount.isReverse));

        struct Vector2s16 edgeDir;
        vector2s16Sub(edgeB, edgeA, &edgeDir);

        if (portalSurfaceIsPointOnLine(point, edgeA, &edgeDir)) {
            surfaceBuilder->hasEdge = 1;
            surfaceBuilder->hasConnected = 1;

            if (point->equalTest == edgeA->equalTest) {
                portalSurfacePrevEdge(surfaceBuilder, &edgeCount, &surfaceBuilder->cuttingEdge);
            } else if (point->equalTest == edgeB->equalTest) {
                surfaceBuilder->cuttingEdge = edgeCount;
            } else {
                if (portalSurfaceSplitEdge(surfaceBuilder, &edgeCount, point) == -1) {
                    return 0;
                }

                surfaceBuilder->cuttingEdge = edgeCount;
            }

            surfaceBuilder->edgeOnSearchLoop = surfaceBuilder->cuttingEdge;

            return 1;
        }

        edgeA = edgeB;
        struct SurfaceEdgeWithSide nextEdge;
        portalSurfaceNextEdge(surfaceBuilder, &edgeCount, &nextEdge);
        edgeCount = nextEdge;
    }

    surfaceBuilder->hasEdge = 0;
    surfaceBuilder->hasConnected = 0;
    surfaceBuilder->startingPoint = portalSurfaceNewVertex(surfaceBuilder, point);

    return 1;
}

int portalSurfaceJoinInnerLoopToOuterLoop(struct PortalSurfaceBuilder* surfaceBuilder) {
    struct SurfaceEdge* outerLoopEdge = portalSurfaceGetEdge(surfaceBuilder, surfaceBuilder->edgeOnSearchLoop.edgeIndex);
    struct Vector2s16* outerLoopPoint = portalSurfaceGetVertex(surfaceBuilder, GET_NEXT_POINT(outerLoopEdge, surfaceBuilder->edgeOnSearchLoop.isReverse));
    
    struct SurfaceEdgeWithSide edgeCount = surfaceBuilder->cuttingEdge;
    struct SurfaceEdgeWithSide nextEdge;

    struct SurfaceEdgeWithSide closestEdge;
    int closestDistance = 0x7FFFFFFF;

    while (portalSurfaceNextEdge(surfaceBuilder, &edgeCount, &nextEdge), nextEdge.edgeIndex != surfaceBuilder->cuttingEdge.edgeIndex) {
        struct Vector2s16* edgePoint = portalSurfaceGetVertex(
            surfaceBuilder, 
            GET_NEXT_POINT(portalSurfaceGetEdge(surfaceBuilder, edgeCount.edgeIndex), edgeCount.isReverse)
        );

        int edgeDistance = vector2s16DistSqr(outerLoopPoint, edgePoint);

        if (edgeDistance < closestDistance) {
            closestDistance = edgeDistance;
            closestEdge = edgeCount;
        }

        edgeCount = nextEdge;
    }

    surfaceBuilder->cuttingEdge = closestEdge;
    return portalSurfaceConnectToPoint(surfaceBuilder, GET_NEXT_POINT(outerLoopEdge, surfaceBuilder->edgeOnSearchLoop.isReverse), &surfaceBuilder->edgeOnSearchLoop);
}

int portalSurfaceIsEdgeUsed(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* edge) {
    return (surfaceBuilder->isEdgeUsed[edge->edgeIndex] & (edge->isReverse ? 0x2 : 0x1)) != 0;
}

void portalSurfaceMarkEdgeUsed(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* edge) {
    surfaceBuilder->isEdgeUsed[edge->edgeIndex] |= edge->isReverse ? 0x2 : 0x1;
}

void portalSurfaceMarkLoopAsUsed(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* edgeOnLoop) {
    struct SurfaceEdgeWithSide edgeCount = *edgeOnLoop;

    int iteration;

    for (iteration = 0; iteration < MAX_SEARCH_ITERATIONS && (iteration == 0 || edgeCount.edgeIndex != edgeOnLoop->edgeIndex || edgeCount.isReverse != edgeOnLoop->isReverse); ++iteration) {
        struct SurfaceEdgeWithSide nextEdge;
        portalSurfaceNextEdge(surfaceBuilder, &edgeCount, &nextEdge);
        portalSurfaceMarkEdgeUsed(surfaceBuilder, &edgeCount);
        edgeCount = nextEdge;
    }
}

void portalSurfaceMarkHoleAsUsed(struct PortalSurfaceBuilder* surfaceBuilder) {
    for (int i = 0; i < surfaceBuilder->edgeCount; ++i) {
        if (surfaceBuilder->isLoopEdge[i]) {
            struct SurfaceEdgeWithSide edgeOnLoop;
            edgeOnLoop.edgeIndex = i;
            edgeOnLoop.isReverse = 1;

            if (!portalSurfaceIsEdgeUsed(surfaceBuilder, &edgeOnLoop)) {
                portalSurfaceMarkLoopAsUsed(surfaceBuilder, &edgeOnLoop);
            }
        }
    }
}

struct PortalSurface* portalSurfaceCutHole(struct PortalSurface* surface, struct Vector2s16* loop) {
    struct PortalSurfaceBuilder surfaceBuilder;

    int edgeCount = surface->edgeCount + ADDITIONAL_EDGE_CAPACITY;

    surfaceBuilder.vertices = stackMalloc(sizeof(struct Vector2s16) * (surface->vertexCount + ADDITIONAL_VERTEX_CAPACITY));
    memCopy(surfaceBuilder.vertices, &surface->vertices, sizeof(struct Vector2s16) * surface->vertexCount);
    surfaceBuilder.vertexCount = surface->vertexCount;
    surfaceBuilder.edges = stackMalloc(sizeof(struct SurfaceEdge) * edgeCount);
    memCopy(surfaceBuilder.edges, &surface->edges, sizeof(struct SurfaceEdge) * surface->edgeCount);
    surfaceBuilder.isLoopEdge = stackMalloc(sizeof(u8) * edgeCount);
    surfaceBuilder.edgeCount = surface->edgeCount;
    surfaceBuilder.isEdgeUsed = stackMalloc(sizeof(u8) *edgeCount);

    zeroMemory(surfaceBuilder.isEdgeUsed, surface->edgeCount + edgeCount);
    zeroMemory(surfaceBuilder.isLoopEdge, surface->edgeCount + edgeCount);

    struct Vector2s16* prev = &loop[0];

    if (!portalSurfaceFindEnclosingFace(surface, prev, &surfaceBuilder.edgeOnSearchLoop)) {
        goto error;
    }

    if (!portalSurfaceFindStartingPoint(&surfaceBuilder, prev)) {
        goto error;
    }

    for (int index = 1; index <= PORTAL_LOOP_SIZE;) {
        struct Vector2s16* next = &loop[index == PORTAL_LOOP_SIZE ? 0 : index];
        struct Vector2s16 dir;

        if (!portalSurfaceFindNextLoop(&surfaceBuilder, next)) {
            goto error;
        }

        vector2s16Sub(next, prev, &dir);
        struct Vector2s16* newPoint = portalSurfaceIntersectEdgeWithLoop(&surfaceBuilder, prev, &dir);

        if (!newPoint) {
            goto error;
        }

        // check if the portal loop ever intersected an edge
        if (index == PORTAL_LOOP_SIZE && !surfaceBuilder.hasConnected) {
            // the portal loop is fully contained
            int firstEdgeIndex = surface->edgeCount;
            int lastEdgeIndex = surfaceBuilder.edgeCount - 1;

            struct SurfaceEdge* firstEdge = portalSurfaceGetEdge(&surfaceBuilder, firstEdgeIndex);
            struct SurfaceEdge* lastEdge = portalSurfaceGetEdge(&surfaceBuilder, lastEdgeIndex);

            firstEdge->prevEdge = lastEdgeIndex;
            firstEdge->nextEdgeReverse = lastEdgeIndex;

            lastEdge->nextEdge = firstEdgeIndex;
            lastEdge->prevEdgeReverse = firstEdgeIndex;

            --surfaceBuilder.vertexCount;

            if (!portalSurfaceJoinInnerLoopToOuterLoop(&surfaceBuilder)) {
                return NULL;
            }
        }

        if (newPoint->equalTest == next->equalTest) {
            // only increment i if the next point was reached
            ++index;
        }

        prev = newPoint;
    }

    portalSurfaceMarkHoleAsUsed(&surfaceBuilder);

    stackMallocFree(surfaceBuilder.isEdgeUsed);
    stackMallocFree(surfaceBuilder.isLoopEdge);
    stackMallocFree(surfaceBuilder.edges);
    stackMallocFree(surfaceBuilder.vertices);

    return NULL;

error:
    stackMallocFree(surfaceBuilder.isEdgeUsed);
    stackMallocFree(surfaceBuilder.isLoopEdge);
    stackMallocFree(surfaceBuilder.edges);
    stackMallocFree(surfaceBuilder.vertices);
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

    portalSurfaceCutHole(surface, portalOutline);

    portalSurfaceInverse(surface, &correctPosition, &portalAt->position);
    // TODO
    // loop through all triangles
    // if no interesctions on triangle pass it through
    // if triangle intersects portal then retrianglute face

    return 1;
}