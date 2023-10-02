#include "portal_surface_generator.h"

#include "portal.h"
#include "../util/memory.h"
#include "../math/mathf.h"
#include "../math/vector2.h"
#include "portal_surface_gfx.h"

#define IS_ORIGINAL_VERTEX_INDEX(surfaceBuilder, vertexIndex) ((vertexIndex) < (surfaceBuilder)->original->vertexCount)

#define MAX_SEARCH_ITERATIONS   32

#define ADDITIONAL_EDGE_CAPACITY 128
#define ADDITIONAL_VERTEX_CAPACITY 32

#define VERIFY_INTEGRITY    1

#define VERY_FAR_AWAY   1e15f

int portalSurfaceFindEnclosingFace(struct PortalSurface* surface, struct Vector2s16* aroundPoint) {
    float minDistance = VERY_FAR_AWAY;
    int result = -1;

    struct Vector2 aroundPointF;

    aroundPointF.x = (float)aroundPoint->x;
    aroundPointF.y = (float)aroundPoint->y;

    for (int i = 0; i < surface->edgeCount; ++i) {
        struct SurfaceEdge* edge = &surface->edges[i];
        struct SurfaceEdge* nextEdge = &surface->edges[edge->nextEdge];

        if (edge->reverseEdge != -1 && edge->pointIndex > nextEdge->pointIndex) {
            // to avoid redundant calcuations only check
            // each pair of  vertcies once
            continue;
        }

        struct Vector2s16* as16 = &surface->vertices[edge->pointIndex];
        struct Vector2s16* bs16 = &surface->vertices[nextEdge->pointIndex];

        struct Vector2 a;
        struct Vector2 b;

        a.x = (float)as16->x;
        a.y = (float)as16->y;

        b.x = (float)bs16->x;
        b.y = (float)bs16->y;

        struct Vector2 edgeOffset;
        struct Vector2 pointOffset;

        vector2Sub(&b, &a, &edgeOffset);
        vector2Sub(&aroundPointF, &a, &pointOffset);

        if (edgeOffset.x == 0.0f && edgeOffset.y == 0.0f) {
            continue;
        }

        float lerp = vector2Dot(&edgeOffset, &pointOffset) / vector2MagSqr(&edgeOffset);

        if (lerp < 0.0f) {
            lerp = 0.0f;
        } else if (lerp > 1.0f) {
            lerp = 1.0f;
        }

        struct Vector2 closestPoint;

        vector2Lerp(&a, &b, lerp, &closestPoint);

        float distSqr = vector2DistSqr(&closestPoint, &aroundPointF);

        if (distSqr < minDistance) {
            minDistance = distSqr;
            result = vector2Cross(&edgeOffset, &pointOffset) < 0.0f ? edge->reverseEdge : i;
        }
    }

    return result;
}

struct SurfaceEdge* portalSurfaceGetEdge(struct PortalSurfaceBuilder* surfaceBuilder, int edgeIndex) {
    return &surfaceBuilder->edges[edgeIndex];
}

struct Vector2s16* portalSurfaceGetVertex(struct PortalSurfaceBuilder* surfaceBuilder, int vertexIndex) {
    return &surfaceBuilder->vertices[vertexIndex];
}

int portalSurfaceNextEdge(struct PortalSurfaceBuilder* surfaceBuilder, int edgeIndex) {
    return surfaceBuilder->edges[edgeIndex].nextEdge;
}

int portalSurfaceNextPoint(struct PortalSurfaceBuilder* surfaceBuilder, int edgeIndex) {
    return surfaceBuilder->edges[surfaceBuilder->edges[edgeIndex].nextEdge].pointIndex;
}

int portalSurfacePrevEdge(struct PortalSurfaceBuilder* surfaceBuilder, int edgeIndex) {
    return surfaceBuilder->edges[edgeIndex].prevEdge;
}

int portalSurfaceReverseEdge(struct PortalSurfaceBuilder* surfaceBuilder, int edgeIndex) {
    return surfaceBuilder->edges[edgeIndex].reverseEdge;
}

#if VERIFY_INTEGRITY

int portalSurfaceIsWellFormed(struct PortalSurfaceBuilder* surfaceBuilder) {
    for (int i = 0; i < surfaceBuilder->currentEdge; ++i) {
        struct SurfaceEdge* edge = portalSurfaceGetEdge(surfaceBuilder, i);

        if (edge->nextEdge == NO_EDGE_CONNECTION) {
            continue;
        }

        struct SurfaceEdge* next = portalSurfaceGetEdge(surfaceBuilder, edge->nextEdge);
        struct SurfaceEdge* prev = portalSurfaceGetEdge(surfaceBuilder, edge->prevEdge);
        struct SurfaceEdge* reverse = edge->reverseEdge == NO_EDGE_CONNECTION ? NULL : portalSurfaceGetEdge(surfaceBuilder, edge->reverseEdge);

        if (next->prevEdge != i || prev->nextEdge != i || (reverse != NULL && reverse->reverseEdge != i)) {
            return 0;
        }
    }

    return 1;
}

#endif

int portalSurfacePointInsideFace(struct PortalSurfaceBuilder* surfaceBuilder, int currentEdge, struct Vector2s16* point) {
    int nextEdge = portalSurfaceNextEdge(surfaceBuilder, currentEdge);

    struct SurfaceEdge* edgePtr = portalSurfaceGetEdge(surfaceBuilder, currentEdge);
    struct SurfaceEdge* nextEdgePtr = portalSurfaceGetEdge(surfaceBuilder, nextEdge);

    struct Vector2s16* corner = portalSurfaceGetVertex(surfaceBuilder, nextEdgePtr->pointIndex);
    struct Vector2s16* prevPoint = portalSurfaceGetVertex(surfaceBuilder, edgePtr->pointIndex);
    struct Vector2s16* nextPoint = portalSurfaceGetVertex(surfaceBuilder, portalSurfaceGetEdge(surfaceBuilder, nextEdgePtr->nextEdge)->pointIndex);

    if (prevPoint->equalTest == nextPoint->equalTest) {
        return 1;
    }

    struct Vector2s16 nextDir;
    struct Vector2s16 prevDir;

    struct Vector2s16 pointDir;

    vector2s16Sub(nextPoint, corner, &nextDir);
    vector2s16Sub(prevPoint, corner, &prevDir);
    vector2s16Sub(point, corner, &pointDir);

    return vector2s16FallsBetween(&nextDir, &prevDir, &pointDir);
}

int portalSurfaceIsEdgeValid(struct PortalSurfaceBuilder* surfaceBuilder, int currentEdge) {
    return surfaceBuilder->edges[currentEdge].nextEdge != NO_EDGE_CONNECTION;
}

// Steps to the next surface around a vertex
int portalSurfaceNextLoop(struct PortalSurfaceBuilder* surfaceBuilder, int currentEdge) {
    int nextFace = portalSurfaceNextEdge(surfaceBuilder, currentEdge);
    nextFace = portalSurfaceReverseEdge(surfaceBuilder, nextFace);

    if (nextFace != NO_EDGE_CONNECTION) {
        return nextFace;
    }

    // hit the edge, the next surface is all the way back
    nextFace = portalSurfaceReverseEdge(surfaceBuilder, currentEdge);

    int i = 0;

    while (i < MAX_SEARCH_ITERATIONS && portalSurfaceReverseEdge(surfaceBuilder, currentEdge) != NO_EDGE_CONNECTION) {
        currentEdge = portalSurfaceReverseEdge(surfaceBuilder, currentEdge);
        currentEdge = portalSurfacePrevEdge(surfaceBuilder, currentEdge);
        ++i;
    }

    return i < MAX_SEARCH_ITERATIONS ? currentEdge : NO_EDGE_CONNECTION;
}

// Find the edge with the associated triangle that contain the point forPoint around a vertex
int portalSurfaceFindCurrentFace(struct PortalSurfaceBuilder* surfaceBuilder, struct Vector2s16* forPoint, int currentEdge) {
    int i = 0;

    while (i < MAX_SEARCH_ITERATIONS && !portalSurfacePointInsideFace(surfaceBuilder, currentEdge, forPoint)) {
        int nextEdge = portalSurfaceNextLoop(surfaceBuilder, currentEdge);

        if (nextEdge == NO_EDGE_CONNECTION) {
            return NO_EDGE_CONNECTION;
        }

        if (i != 0 && currentEdge == surfaceBuilder->edgeOnSearchLoop) {
            // a full loop and no face found
            return NO_EDGE_CONNECTION;
        }

        currentEdge = nextEdge;
        ++i;
    }

    return i < MAX_SEARCH_ITERATIONS ? currentEdge : NO_EDGE_CONNECTION;
}

int portalSurfaceFindNextLoop(struct PortalSurfaceBuilder* surfaceBuilder, struct Vector2s16* forPoint) {
    if (!surfaceBuilder->hasConnected) {
        return 1;
    }

    int currentEdge = surfaceBuilder->edgeOnSearchLoop;

    int nextIndex = portalSurfaceGetEdge(surfaceBuilder, surfaceBuilder->cuttingEdge)->nextEdge;

    // the cutting edge is extended into a face already
    // this means edgeOnSearchLoop will already be correct
    if (portalSurfaceGetEdge(surfaceBuilder, nextIndex)->reverseEdge == surfaceBuilder->cuttingEdge) {
        return 1;
    }

    currentEdge = portalSurfaceFindCurrentFace(surfaceBuilder, forPoint, currentEdge);

    if (currentEdge == NO_EDGE_CONNECTION) {
        return 0;
    }

    surfaceBuilder->edgeOnSearchLoop = currentEdge;
    surfaceBuilder->cuttingEdge = currentEdge;

    return 1;
}

#define MAX_INTERSECT_LOOPS 20

int portalSurfaceIsPointOnLine(struct Vector2s16* pointA, struct Vector2s16* edgeA, struct Vector2s16* edgeDir) {
    if (edgeDir->equalTest == 0) {
        return 0;
    }

    struct Vector2s16 originOffset;
    vector2s16Sub(edgeA, pointA, &originOffset);

    int crossProduct = vector2s16Cross(&originOffset, edgeDir);

    if (crossProduct != 0) {
        return 0;
    }

    int dotProduct = vector2s16Dot(&originOffset, edgeDir);
    int edgeDirLength = vector2s16MagSqr(edgeDir);

    return dotProduct >= 0 && dotProduct <= edgeDirLength;
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
                // point ends first
                vector2s16Add(pointA, pointDir, intersection);
            } else {
                // edge ends first
                *intersection = *edgeB;
            }
        } else {
            if (vector2s16MagSqr(&originOffset) >= vector2s16MagSqr(pointDir)) {
                // point ends first
                vector2s16Add(pointA, pointDir, intersection);
            } else {
                // edge ends first
                *intersection = *edgeA;
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

    if (denominator > 0 ? (edgeLerp <= 0 || edgeLerp > denominator) : (edgeLerp >= 0 || edgeLerp < denominator)) {
        return IntersectionTypeNone;
    }

    intersection->x = (short)(((s64)edgeDir.x * (s64)edgeLerp) / (s64)denominator) + edgeA->x;
    intersection->y = (short)(((s64)edgeDir.y * (s64)edgeLerp) / (s64)denominator) + edgeA->y;

    return IntersectionTypePoint;
}

int portalSurfaceNewVertex(struct PortalSurfaceBuilder* surfaceBuilder, struct Vector2s16* point) {
    if (surfaceBuilder->currentVertex == ADDITIONAL_VERTEX_CAPACITY + surfaceBuilder->original->vertexCount) {
        return -1;
    }

    int newVertexIndex = surfaceBuilder->currentVertex;
    surfaceBuilder->vertices[newVertexIndex] = *point;
    ++surfaceBuilder->currentVertex;

    return newVertexIndex;
}

int portalSurfaceNewEdge(struct PortalSurfaceBuilder* surfaceBuilder, int isLoopEdge, int originalEdge) {
    while (surfaceBuilder->checkForEdgeReuse < surfaceBuilder->currentEdge) {
        struct SurfaceEdge* edge = portalSurfaceGetEdge(surfaceBuilder, surfaceBuilder->checkForEdgeReuse);

        if (edge->nextEdge == NO_EDGE_CONNECTION) {
            surfaceBuilder->isLoopEdge[surfaceBuilder->checkForEdgeReuse] = isLoopEdge;
            surfaceBuilder->originalEdgeIndex[surfaceBuilder->checkForEdgeReuse] = originalEdge;
            // ensure this isn't returned on the next call
            edge->nextEdge = 0;

            return surfaceBuilder->checkForEdgeReuse;
        }

        ++surfaceBuilder->checkForEdgeReuse;
    }

    if (surfaceBuilder->currentEdge == ADDITIONAL_EDGE_CAPACITY + surfaceBuilder->original->edgeCount) {
        return -1;
    }

    int newEdgeIndex = surfaceBuilder->currentEdge;
    surfaceBuilder->isLoopEdge[surfaceBuilder->currentEdge] = isLoopEdge;
    surfaceBuilder->originalEdgeIndex[newEdgeIndex] = originalEdge;
    ++surfaceBuilder->currentEdge;
    ++surfaceBuilder->checkForEdgeReuse;
    return newEdgeIndex;
}

void portalSurfaceLerpVtx(struct PortalSurfaceBuilder* surfaceBuilder, int aIndex, int bIndex, int resultIndex) {
    int numerator = 0;
    int deniminator = 0;

    struct Vector2s16* aPos = &surfaceBuilder->vertices[aIndex];
    struct Vector2s16* bPos = &surfaceBuilder->vertices[bIndex];
    struct Vector2s16* resultPos = &surfaceBuilder->vertices[resultIndex];

    int xOffset = bPos->x - aPos->x;
    int yOffset = bPos->y - aPos->y;

    if (abs(xOffset) > abs(yOffset)) {
        numerator = resultPos->x - aPos->x;
        deniminator = xOffset;
    } else {
        numerator = resultPos->y - aPos->y;
        deniminator = yOffset;
    }

    float lerp = (float)numerator / (float)deniminator;

    Vtx* a = &surfaceBuilder->gfxVertices[aIndex];
    Vtx* b = &surfaceBuilder->gfxVertices[bIndex];
    Vtx* result = &surfaceBuilder->gfxVertices[resultIndex];

    result->v.ob[0] = (short)mathfLerp(a->v.ob[0], b->v.ob[0], lerp);
    result->v.ob[1] = (short)mathfLerp(a->v.ob[1], b->v.ob[1], lerp);
    result->v.ob[2] = (short)mathfLerp(a->v.ob[2], b->v.ob[2], lerp);

    result->v.flag = 0;

    result->v.tc[0] = (short)mathfLerp(a->v.tc[0], b->v.tc[0], lerp);
    result->v.tc[1] = (short)mathfLerp(a->v.tc[1], b->v.tc[1], lerp);

    result->v.cn[0] = (short)mathfLerp(a->v.cn[0], b->v.cn[0], lerp);
    result->v.cn[1] = (short)mathfLerp(a->v.cn[1], b->v.cn[1], lerp);
    result->v.cn[2] = (short)mathfLerp(a->v.cn[2], b->v.cn[2], lerp);
    result->v.cn[3] = (short)mathfLerp(a->v.cn[3], b->v.cn[3], lerp);
}

void portalSurfaceCalcVertex(struct PortalSurfaceBuilder* surfaceBuilder, int loopEdge, int resultIndex) {
    int originalEdge = surfaceBuilder->originalEdgeIndex[loopEdge];

    struct SurfaceEdge* originalEdgePtr = &surfaceBuilder->original->edges[originalEdge];
    int nextEdge = originalEdgePtr->nextEdge;

    struct SurfaceEdge* nextEdgePtr = &surfaceBuilder->original->edges[nextEdge];

    int aIndex = originalEdgePtr->pointIndex;
    int bIndex = nextEdgePtr->pointIndex;
    int cIndex = surfaceBuilder->original->edges[nextEdgePtr->nextEdge].pointIndex;

    struct Vector3 barycentericCoords;

    vector2s16Barycentric(
        &surfaceBuilder->vertices[aIndex], 
        &surfaceBuilder->vertices[bIndex], 
        &surfaceBuilder->vertices[cIndex], 
        &surfaceBuilder->vertices[resultIndex], 
        &barycentericCoords
    );

    Vtx* a = &surfaceBuilder->gfxVertices[aIndex];
    Vtx* b = &surfaceBuilder->gfxVertices[bIndex];
    Vtx* c = &surfaceBuilder->gfxVertices[cIndex];
    Vtx* result = &surfaceBuilder->gfxVertices[resultIndex];

    result->v.ob[0] = (short)vector3EvalBarycentric1D(&barycentericCoords, a->v.ob[0], b->v.ob[0], c->v.ob[0]);
    result->v.ob[1] = (short)vector3EvalBarycentric1D(&barycentericCoords, a->v.ob[1], b->v.ob[1], c->v.ob[1]);
    result->v.ob[2] = (short)vector3EvalBarycentric1D(&barycentericCoords, a->v.ob[2], b->v.ob[2], c->v.ob[2]);

    result->v.flag = 0;

    result->v.tc[0] = (short)vector3EvalBarycentric1D(&barycentericCoords, a->v.tc[0], b->v.tc[0], c->v.tc[0]);
    result->v.tc[1] = (short)vector3EvalBarycentric1D(&barycentericCoords, a->v.tc[1], b->v.tc[1], c->v.tc[1]);

    result->v.cn[0] = (short)vector3EvalBarycentric1D(&barycentericCoords, a->v.cn[0], b->v.cn[0], c->v.cn[0]);
    result->v.cn[1] = (short)vector3EvalBarycentric1D(&barycentericCoords, a->v.cn[1], b->v.cn[1], c->v.cn[1]);
    result->v.cn[2] = (short)vector3EvalBarycentric1D(&barycentericCoords, a->v.cn[2], b->v.cn[2], c->v.cn[2]);
    result->v.cn[3] = (short)vector3EvalBarycentric1D(&barycentericCoords, a->v.cn[3], b->v.cn[3], c->v.cn[3]);
}

int portalSurfaceSplitEdgeWithVertex(struct PortalSurfaceBuilder* surfaceBuilder, int edge, int newVertexIndex) {
    struct SurfaceEdge* existingEdge = portalSurfaceGetEdge(surfaceBuilder, edge);

    int newEdgeIndex = portalSurfaceNewEdge(surfaceBuilder, 0, surfaceBuilder->originalEdgeIndex[edge]);

    if (newEdgeIndex == -1) {
        return 0;
    }

    struct SurfaceEdge* newEdge = portalSurfaceGetEdge(surfaceBuilder, newEdgeIndex);
    newEdge->pointIndex = newVertexIndex;

    int newReverseEdgeIndex = NO_EDGE_CONNECTION;
    struct SurfaceEdge* newReverseEdge = NULL;
    struct SurfaceEdge* existingReverseEdge = NULL;

    int existingReverseEdgeIndex = existingEdge->reverseEdge;
    
    if (existingReverseEdgeIndex != NO_EDGE_CONNECTION) {
        newReverseEdgeIndex = portalSurfaceNewEdge(surfaceBuilder, 0, surfaceBuilder->originalEdgeIndex[existingReverseEdgeIndex]);

        if (newReverseEdgeIndex == -1) {
            return 0;
        }
        
        newReverseEdge = portalSurfaceGetEdge(surfaceBuilder, newReverseEdgeIndex);

        newReverseEdge->pointIndex = newVertexIndex;

        existingReverseEdge = portalSurfaceGetEdge(surfaceBuilder, existingReverseEdgeIndex);

        existingReverseEdge->reverseEdge = newEdgeIndex;
        newEdge->reverseEdge = existingReverseEdgeIndex;

        existingEdge->reverseEdge = newReverseEdgeIndex;
        newReverseEdge->reverseEdge = edge;

    } else {
        newEdge->reverseEdge = NO_EDGE_CONNECTION;
    }

    newEdge->nextEdge = existingEdge->nextEdge;
    newEdge->prevEdge = edge;

    existingEdge->nextEdge = newEdgeIndex;
    portalSurfaceGetEdge(surfaceBuilder, newEdge->nextEdge)->prevEdge = newEdgeIndex;

    if (newReverseEdge) {
        newReverseEdge->nextEdge = existingReverseEdge->nextEdge;
        newReverseEdge->prevEdge = existingReverseEdgeIndex;

        existingReverseEdge->nextEdge = newReverseEdgeIndex;
        portalSurfaceGetEdge(surfaceBuilder, newReverseEdge->nextEdge)->prevEdge = newReverseEdgeIndex;
    }

    return 1;
}

int portalSurfaceSplitEdge(struct PortalSurfaceBuilder* surfaceBuilder, int edge, struct Vector2s16* point) {
    int newVertexIndex = portalSurfaceNewVertex(surfaceBuilder, point);
    
    if (newVertexIndex == -1) {
        return -1;
    }

    struct SurfaceEdge* existingEdge = portalSurfaceGetEdge(surfaceBuilder, edge);
    struct SurfaceEdge* nextExistingEdge = portalSurfaceGetEdge(surfaceBuilder, existingEdge->nextEdge);

    portalSurfaceLerpVtx(surfaceBuilder, existingEdge->pointIndex, nextExistingEdge->pointIndex, newVertexIndex);

    if (!portalSurfaceSplitEdgeWithVertex(surfaceBuilder, edge, newVertexIndex)) {
        return -1;
    }

#if VERIFY_INTEGRITY
    if (!portalSurfaceIsWellFormed(surfaceBuilder)) {
        return 0;
    }
#endif

    return newVertexIndex;
}

int portalSurfaceConnectToPoint(struct PortalSurfaceBuilder* surfaceBuilder, int pointIndex, int edgeIndex, int isLoopEdge) {
    int originalEdge = surfaceBuilder->originalEdgeIndex[surfaceBuilder->edgeOnSearchLoop];

    int newEdge = -1;
    int newReverseEdge = -1;

    if (surfaceBuilder->hasEdge && surfaceBuilder->vertices[portalSurfaceNextPoint(surfaceBuilder, surfaceBuilder->cuttingEdge)].equalTest == surfaceBuilder->vertices[pointIndex].equalTest) {
        // cutting edge is already directly on new point just not connected
        // this sets up the state to connect it properly
        struct SurfaceEdge* newEdgePtr = portalSurfaceGetEdge(surfaceBuilder, surfaceBuilder->cuttingEdge);

        // reuse previous edges
        newEdge = surfaceBuilder->cuttingEdge;
        newReverseEdge = newEdgePtr->reverseEdge;

        if (newEdgePtr->prevEdge == newEdgePtr->reverseEdge) {
            // connect from the starting point
            surfaceBuilder->hasEdge = 0;
            surfaceBuilder->startingPoint = newEdgePtr->pointIndex;
        } else {
            // back up to new cuttingEdge
            surfaceBuilder->cuttingEdge = newEdgePtr->prevEdge;

            struct SurfaceEdge* reverseEdgePtr = portalSurfaceGetEdge(surfaceBuilder, newEdgePtr->reverseEdge);

            // rejoin previous cuttingEdge 
            portalSurfaceGetEdge(surfaceBuilder, surfaceBuilder->cuttingEdge)->nextEdge = reverseEdgePtr->nextEdge;
            portalSurfaceGetEdge(surfaceBuilder, reverseEdgePtr->nextEdge)->prevEdge = surfaceBuilder->cuttingEdge;
        }
    } else if (!surfaceBuilder->hasEdge && surfaceBuilder->vertices[surfaceBuilder->startingPoint].equalTest == surfaceBuilder->vertices[pointIndex].equalTest) {
        // starting point is directly on edge use edage as new cutting edge
        surfaceBuilder->hasEdge = 1;
        surfaceBuilder->cuttingEdge = edgeIndex;
        surfaceBuilder->edgeOnSearchLoop = edgeIndex;
        return 1;
    } else {
        newEdge = portalSurfaceNewEdge(surfaceBuilder, 0, originalEdge);
        newReverseEdge = portalSurfaceNewEdge(surfaceBuilder, isLoopEdge, originalEdge);
    }


    if (newEdge == -1 || newReverseEdge == -1) {
        return 0;
    }

    struct SurfaceEdge* newEdgePtr = portalSurfaceGetEdge(surfaceBuilder, newEdge);
    struct SurfaceEdge* newReverseEdgePtr = portalSurfaceGetEdge(surfaceBuilder, newReverseEdge);

    newEdgePtr->reverseEdge = newReverseEdge;
    newReverseEdgePtr->reverseEdge = newEdge;

    if (surfaceBuilder->hasEdge) {
        int nextEdge = portalSurfaceNextEdge(surfaceBuilder, surfaceBuilder->cuttingEdge);

        struct SurfaceEdge* cuttingEdgePtr = portalSurfaceGetEdge(surfaceBuilder, surfaceBuilder->cuttingEdge);
        struct SurfaceEdge* nextEdgePtr = portalSurfaceGetEdge(surfaceBuilder, nextEdge);

        newEdgePtr->pointIndex = nextEdgePtr->pointIndex;

        newEdgePtr->prevEdge = surfaceBuilder->cuttingEdge;
        newReverseEdgePtr->nextEdge = nextEdge;

        cuttingEdgePtr->nextEdge = newEdge;
        nextEdgePtr->prevEdge = newReverseEdge;
    } else {
        newEdgePtr->prevEdge = newReverseEdge;
        newReverseEdgePtr->nextEdge = newEdge;
        newEdgePtr->pointIndex = surfaceBuilder->startingPoint;
    }

    newReverseEdgePtr->pointIndex = pointIndex;

    if (edgeIndex != NO_EDGE_CONNECTION) {
        int nextEdge = portalSurfaceNextEdge(surfaceBuilder, edgeIndex);

        struct SurfaceEdge* edgePtr = portalSurfaceGetEdge(surfaceBuilder, edgeIndex);
        struct SurfaceEdge* nextEdgePtr = portalSurfaceGetEdge(surfaceBuilder, nextEdge);

        newEdgePtr->nextEdge = nextEdge;
        newReverseEdgePtr->prevEdge = edgeIndex;

        edgePtr->nextEdge = newReverseEdge;
        nextEdgePtr->prevEdge = newEdge;
    } else {
        newEdgePtr->nextEdge = newReverseEdge;
        newReverseEdgePtr->prevEdge = newEdge;
    }

    surfaceBuilder->hasEdge = 1;
    surfaceBuilder->cuttingEdge = newEdge;

#if VERIFY_INTEGRITY
    if (!portalSurfaceIsWellFormed(surfaceBuilder)) {
        return 0;
    }
#endif

    return 1;
}

#define COLLAPSE_DISTANCE   8

struct Vector2s16* portalSurfaceIntersectEdgeWithLoop(struct PortalSurfaceBuilder* surfaceBuilder, struct Vector2s16* pointA, struct Vector2s16* pointB, int isFinalPoint) {
    int currentEdge = surfaceBuilder->edgeOnSearchLoop;

    struct Vector2s16 pointDir;
    vector2s16Sub(pointB, pointA, &pointDir);

    int iteration;

    for (iteration = 0; 
        iteration < MAX_INTERSECT_LOOPS && (
            iteration == 0 || 
            currentEdge != surfaceBuilder->edgeOnSearchLoop
        ); (currentEdge = portalSurfaceNextEdge(surfaceBuilder, currentEdge)), ++iteration) {
            
        if (!isFinalPoint && surfaceBuilder->isLoopEdge[currentEdge]) {
            continue;
        }

        struct SurfaceEdge* edge = portalSurfaceGetEdge(surfaceBuilder, currentEdge);

        int nextPointIndex = surfaceBuilder->edges[edge->nextEdge].pointIndex;

        struct Vector2s16* edgeA = portalSurfaceGetVertex(surfaceBuilder, edge->pointIndex);
        struct Vector2s16* edgeB = portalSurfaceGetVertex(surfaceBuilder, nextPointIndex);

        struct Vector2s16 intersectionPoint;

        enum IntersectionType intersectType = portalSurfaceIntersect(pointA, &pointDir, edgeA, edgeB, &intersectionPoint);

        if (intersectType == IntersectionTypePoint) {
            int newPointIndex;

            if (intersectionPoint.equalTest == edgeA->equalTest) {
                newPointIndex = edge->pointIndex;
                currentEdge = portalSurfaceFindCurrentFace(surfaceBuilder, pointA, currentEdge);

                if (currentEdge == NO_EDGE_CONNECTION) {
                    return NULL;
                }
            } else if (intersectionPoint.equalTest == edgeB->equalTest) {
                newPointIndex = nextPointIndex;
                currentEdge = portalSurfaceFindCurrentFace(surfaceBuilder, pointA, currentEdge);

                if (currentEdge == NO_EDGE_CONNECTION) {
                    return NULL;
                }
            } else {
                newPointIndex = portalSurfaceSplitEdge(surfaceBuilder, currentEdge, &intersectionPoint);
                
                if (newPointIndex == -1) {
                    return NULL;
                }
            }

            if (!portalSurfaceConnectToPoint(surfaceBuilder, newPointIndex, currentEdge, 1)) {
                return NULL;
            }

            surfaceBuilder->hasConnected = 1;
            surfaceBuilder->edgeOnSearchLoop = surfaceBuilder->cuttingEdge;

            return portalSurfaceGetVertex(surfaceBuilder, newPointIndex);
        } else if (intersectType == IntersectionTypeColinear) {
            int newPointIndex;

            if (intersectionPoint.equalTest == edgeA->equalTest) {
                newPointIndex = edge->pointIndex;
                surfaceBuilder->cuttingEdge = portalSurfacePrevEdge(surfaceBuilder, currentEdge);
            } else if (intersectionPoint.equalTest == edgeB->equalTest) {
                newPointIndex = nextPointIndex;
                surfaceBuilder->cuttingEdge = currentEdge;
            } else {
                newPointIndex = portalSurfaceSplitEdge(surfaceBuilder, currentEdge, &intersectionPoint);

                if (newPointIndex == -1) {
                    return NULL;
                }

                surfaceBuilder->cuttingEdge = currentEdge;
            }

            surfaceBuilder->hasEdge = 1;
            surfaceBuilder->hasConnected = 1;

            return portalSurfaceGetVertex(surfaceBuilder, newPointIndex);
        }
    }

    int newPointIndex = portalSurfaceNewVertex(surfaceBuilder, pointB);

    portalSurfaceCalcVertex(surfaceBuilder, surfaceBuilder->edgeOnSearchLoop, newPointIndex);

    if (!portalSurfaceConnectToPoint(surfaceBuilder, newPointIndex, NO_EDGE_CONNECTION, 1)) {
        return NULL;
    }

    return portalSurfaceGetVertex(surfaceBuilder, newPointIndex);
}

int portalSurfaceFindStartingPoint(struct PortalSurfaceBuilder* surfaceBuilder, struct Vector2s16* point) {
    int currentEdge = surfaceBuilder->edgeOnSearchLoop;

    struct Vector2s16* edgeA = portalSurfaceGetVertex(surfaceBuilder, portalSurfaceGetEdge(surfaceBuilder, currentEdge)->pointIndex);

    for (int iteration = 0; iteration < MAX_INTERSECT_LOOPS; ++iteration) {
        if (iteration > 0 && currentEdge == surfaceBuilder->edgeOnSearchLoop) {
            // finished searching loop
            break;
        }

        struct SurfaceEdge* edge = portalSurfaceGetEdge(surfaceBuilder, currentEdge);

        struct Vector2s16* edgeB = portalSurfaceGetVertex(surfaceBuilder, portalSurfaceGetEdge(surfaceBuilder, edge->nextEdge)->pointIndex);

        struct Vector2s16 edgeDir;
        vector2s16Sub(edgeB, edgeA, &edgeDir);

        if (portalSurfaceIsPointOnLine(point, edgeA, &edgeDir)) {
            surfaceBuilder->hasEdge = 1;
            surfaceBuilder->hasConnected = 1;

            if (point->equalTest == edgeA->equalTest) {
                surfaceBuilder->cuttingEdge = portalSurfacePrevEdge(surfaceBuilder, currentEdge);
            } else if (point->equalTest == edgeB->equalTest) {
                surfaceBuilder->cuttingEdge = currentEdge;
            } else {
                if (portalSurfaceSplitEdge(surfaceBuilder, currentEdge, point) == -1) {
                    return 0;
                }

                surfaceBuilder->cuttingEdge = currentEdge;
            }

            surfaceBuilder->edgeOnSearchLoop = surfaceBuilder->cuttingEdge;

            return 1;
        }

        edgeA = edgeB;
        currentEdge = portalSurfaceNextEdge(surfaceBuilder, currentEdge);
    }

    surfaceBuilder->hasEdge = 0;
    surfaceBuilder->hasConnected = 0;
    surfaceBuilder->startingPoint = portalSurfaceNewVertex(surfaceBuilder, point);

    portalSurfaceCalcVertex(surfaceBuilder, surfaceBuilder->edgeOnSearchLoop, surfaceBuilder->startingPoint);

    return 1;
}

int portalSurfaceJoinInnerLoopToOuterLoop(struct PortalSurfaceBuilder* surfaceBuilder) {
    struct SurfaceEdge* outerLoopEdge = portalSurfaceGetEdge(surfaceBuilder, surfaceBuilder->edgeOnSearchLoop);
    struct Vector2s16* outerLoopPoint = portalSurfaceGetVertex(surfaceBuilder, portalSurfaceGetEdge(surfaceBuilder, outerLoopEdge->nextEdge)->pointIndex);
    
    int currentEdge = surfaceBuilder->cuttingEdge;
    int nextEdge;

    int closestEdge = currentEdge;
    int closestDistance = 0x7FFFFFFF;

    while ((nextEdge = portalSurfaceNextEdge(surfaceBuilder, currentEdge)), nextEdge != surfaceBuilder->cuttingEdge) {
        struct Vector2s16* edgePoint = portalSurfaceGetVertex(
            surfaceBuilder, 
            portalSurfaceNextPoint(surfaceBuilder, currentEdge)
        );

        int edgeDistance = vector2s16DistSqr(outerLoopPoint, edgePoint);

        if (edgeDistance < closestDistance) {
            closestDistance = edgeDistance;
            closestEdge = currentEdge;
        }

        currentEdge = nextEdge;
    }

    surfaceBuilder->cuttingEdge = closestEdge;
    return portalSurfaceConnectToPoint(surfaceBuilder, portalSurfaceGetEdge(surfaceBuilder, outerLoopEdge->nextEdge)->pointIndex, surfaceBuilder->edgeOnSearchLoop, 0);
}

int portalSurfaceHasFlag(struct PortalSurfaceBuilder* surfaceBuilder, int edgeIndex, enum SurfaceEdgeFlags value) {
    return (surfaceBuilder->edgeFlags[edgeIndex] & value) != 0;
}

void portalSurfaceSetFlag(struct PortalSurfaceBuilder* surfaceBuilder, int edgeIndex, enum SurfaceEdgeFlags value) {
    surfaceBuilder->edgeFlags[edgeIndex] |= value;
}

void portalSurfaceMarkLoopAsUsed(struct PortalSurfaceBuilder* surfaceBuilder, int edgeOnLoop) {
    int currentEdge = edgeOnLoop;

    int iteration;

    for (iteration = 0; iteration < MAX_SEARCH_ITERATIONS && (iteration == 0 || currentEdge != edgeOnLoop); ++iteration) {
        int nextEdge = portalSurfaceNextEdge(surfaceBuilder, currentEdge);

        portalSurfaceSetFlag(surfaceBuilder, currentEdge, SurfaceEdgeFlagsUsed);

        int reverseEdge = surfaceBuilder->edges[currentEdge].reverseEdge;

        if (reverseEdge != NO_EDGE_CONNECTION) {
            surfaceBuilder->edges[reverseEdge].reverseEdge = NO_EDGE_CONNECTION;
        }

        surfaceBuilder->edges[currentEdge].nextEdge = NO_EDGE_CONNECTION;
        surfaceBuilder->edges[currentEdge].prevEdge = NO_EDGE_CONNECTION;
        surfaceBuilder->edges[currentEdge].reverseEdge = NO_EDGE_CONNECTION;
        
        struct SurfaceEdge* surfaceEdge = portalSurfaceGetEdge(surfaceBuilder, nextEdge);

        // since this function clears out nextEdge and prevEdge
        // portalSurfaceNextEdge will fail after the loop as been
        // marked. If this check fails the loop is finished
        if (surfaceEdge->prevEdge != currentEdge) {
            return;
        }

        currentEdge = nextEdge;
    }
}

void portalSurfaceMarkHoleAsUsed(struct PortalSurfaceBuilder* surfaceBuilder) {
    for (int i = 0; i < surfaceBuilder->currentEdge; ++i) {
        if (surfaceBuilder->isLoopEdge[i]) {
            if (!portalSurfaceHasFlag(surfaceBuilder, i, SurfaceEdgeFlagsUsed)) {
                portalSurfaceMarkLoopAsUsed(surfaceBuilder, i);
            }
        }
    }

    // setting this causes the algorithm to reuse edges when running the
    // triangulate algorithm
    surfaceBuilder->checkForEdgeReuse = 0;
}

int portalSurfaceConnectEdges(struct PortalSurfaceBuilder* surfaceBuilder, int from, int to) {
    struct SurfaceEdge* nextEdgePtr = portalSurfaceGetEdge(surfaceBuilder, to);
    surfaceBuilder->cuttingEdge = from;
    surfaceBuilder->hasEdge = 1;
    if (!portalSurfaceConnectToPoint(surfaceBuilder, portalSurfaceGetEdge(surfaceBuilder, nextEdgePtr->nextEdge)->pointIndex, to, 0)) {
        return NO_EDGE_CONNECTION;
    }
    return surfaceBuilder->cuttingEdge;
}

int portalSurfaceTriangulateLoop(struct PortalSurfaceBuilder* surfaceBuilder, int edgeOnLoop) {
    if (portalSurfaceHasFlag(surfaceBuilder, edgeOnLoop, SurfaceEdgeFlagsTriangulated)) {
        return 1;
    }

    int currentEdge = edgeOnLoop;
    int iteration = 0;
    struct SurfaceEdge* currentEdgePtr = portalSurfaceGetEdge(surfaceBuilder, currentEdge);

    // Find the starting point
    while (!IS_ORIGINAL_VERTEX_INDEX(surfaceBuilder, currentEdgePtr->pointIndex) || IS_ORIGINAL_VERTEX_INDEX(surfaceBuilder, portalSurfaceGetEdge(surfaceBuilder, currentEdgePtr->nextEdge)->pointIndex)) {
        ++iteration;
        currentEdge = portalSurfaceNextEdge(surfaceBuilder, currentEdge);
        currentEdgePtr = portalSurfaceGetEdge(surfaceBuilder, currentEdge);

        if (iteration == MAX_SEARCH_ITERATIONS || currentEdge == edgeOnLoop) {
            if (iteration == 3) {
                // already a triangle

                int nextEdge = portalSurfaceNextEdge(surfaceBuilder, currentEdge);
                int prevEdge = portalSurfacePrevEdge(surfaceBuilder, currentEdge);
                
                portalSurfaceSetFlag(surfaceBuilder, currentEdge, SurfaceEdgeFlagsTriangulated);
                portalSurfaceSetFlag(surfaceBuilder, nextEdge, SurfaceEdgeFlagsTriangulated);
                portalSurfaceSetFlag(surfaceBuilder, prevEdge, SurfaceEdgeFlagsTriangulated);
                return 1;

            }

            return 0;
        }
    }

    for (iteration = 0; iteration < MAX_SEARCH_ITERATIONS; ++iteration) {
        currentEdgePtr = portalSurfaceGetEdge(surfaceBuilder, currentEdge);

        struct Vector2s16* edgePointA = portalSurfaceGetVertex(surfaceBuilder, currentEdgePtr->pointIndex);
        struct Vector2s16* edgePointB = portalSurfaceGetVertex(surfaceBuilder, portalSurfaceGetEdge(surfaceBuilder, currentEdgePtr->nextEdge)->pointIndex);

        int nextEdge = portalSurfaceNextEdge(surfaceBuilder, currentEdge);
        int prevEdge = portalSurfacePrevEdge(surfaceBuilder, currentEdge);

        int prevPointIndex = portalSurfaceGetEdge(surfaceBuilder, prevEdge)->pointIndex;
        
        struct Vector2s16* nextPoint = portalSurfaceGetVertex(surfaceBuilder, portalSurfaceNextPoint(surfaceBuilder, nextEdge));
        struct Vector2s16* prevPoint = portalSurfaceGetVertex(surfaceBuilder, prevPointIndex);

        // check if finished
        if (nextPoint == prevPoint) {
            portalSurfaceSetFlag(surfaceBuilder, currentEdge, SurfaceEdgeFlagsTriangulated);
            portalSurfaceSetFlag(surfaceBuilder, nextEdge, SurfaceEdgeFlagsTriangulated);
            portalSurfaceSetFlag(surfaceBuilder, prevEdge, SurfaceEdgeFlagsTriangulated);
            return 1;
        }

        struct Vector2s16 edgeDir;
        struct Vector2s16 nextEdgeDir;
        struct Vector2s16 prevEdgeDir;

        vector2s16Sub(edgePointB, edgePointA, &edgeDir);
        vector2s16Sub(nextPoint, edgePointB, &nextEdgeDir);
        vector2s16Sub(edgePointB, prevPoint, &prevEdgeDir);

        int nextCurrentEdge;
        if (IS_ORIGINAL_VERTEX_INDEX(surfaceBuilder, prevPointIndex) && vector2s16Cross(&prevEdgeDir, &nextEdgeDir) > vector2s16Cross(&edgeDir, &nextEdgeDir)) {
            int prevPrevEdge = portalSurfacePrevEdge(surfaceBuilder, prevEdge);

            nextCurrentEdge = portalSurfaceConnectEdges(surfaceBuilder, prevPrevEdge, currentEdge);

            if (nextCurrentEdge == NO_EDGE_CONNECTION) {
                return 0;
            }

            portalSurfaceSetFlag(surfaceBuilder, prevEdge, SurfaceEdgeFlagsTriangulated);
            portalSurfaceSetFlag(surfaceBuilder, currentEdge, SurfaceEdgeFlagsTriangulated);
        } else {
            nextCurrentEdge = portalSurfaceConnectEdges(surfaceBuilder, prevEdge, nextEdge);

            if (nextCurrentEdge == NO_EDGE_CONNECTION) {
                return 0;
            }

            portalSurfaceSetFlag(surfaceBuilder, currentEdge, SurfaceEdgeFlagsTriangulated);
            portalSurfaceSetFlag(surfaceBuilder, nextEdge, SurfaceEdgeFlagsTriangulated);
        }

        currentEdge = nextCurrentEdge;

        nextCurrentEdge = portalSurfaceReverseEdge(surfaceBuilder, nextCurrentEdge);

        portalSurfaceSetFlag(surfaceBuilder, nextCurrentEdge, SurfaceEdgeFlagsTriangulated);
    }

    return 0;
}

int portalSurfaceTriangulate(struct PortalSurfaceBuilder* surfaceBuilder) {
    for (int i = 0; i < surfaceBuilder->currentEdge; ++i) {
        if (portalSurfaceGetEdge(surfaceBuilder, i)->nextEdge == NO_EDGE_CONNECTION) {
            continue;
        }

        if (!portalSurfaceTriangulateLoop(surfaceBuilder, i)) {
            return 0;
        }
    }

    return 1;
}

int portalSurfacePokeHole(struct PortalSurface* surface, struct Vector2s16* loop, struct PortalSurface* result) {
    struct PortalSurfaceBuilder surfaceBuilder;

    int edgeCapacity = surface->edgeCount + ADDITIONAL_EDGE_CAPACITY;

    surfaceBuilder.original = surface;
    surfaceBuilder.vertices = stackMalloc(sizeof(struct Vector2s16) * (surface->vertexCount + ADDITIONAL_VERTEX_CAPACITY));
    memCopy(surfaceBuilder.vertices, surface->vertices, sizeof(struct Vector2s16) * surface->vertexCount);
    surfaceBuilder.currentVertex = surface->vertexCount;
    surfaceBuilder.edges = stackMalloc(sizeof(struct SurfaceEdge) * edgeCapacity);
    memCopy(surfaceBuilder.edges, surface->edges, sizeof(struct SurfaceEdge) * surface->edgeCount);
    surfaceBuilder.isLoopEdge = stackMalloc(sizeof(u8) * edgeCapacity);
    surfaceBuilder.currentEdge = surface->edgeCount;
    surfaceBuilder.checkForEdgeReuse = surface->edgeCount;
    surfaceBuilder.edgeFlags = stackMalloc(sizeof(u8) * edgeCapacity);
    surfaceBuilder.originalEdgeIndex = stackMalloc(sizeof(u8) * edgeCapacity);

    for (int i = 0; i < surface->edgeCount; ++i) {
        surfaceBuilder.originalEdgeIndex[i] = i;
    }

    surfaceBuilder.gfxVertices = stackMalloc(sizeof(Vtx) * (surface->vertexCount + ADDITIONAL_EDGE_CAPACITY));

    zeroMemory(surfaceBuilder.edgeFlags, edgeCapacity);
    zeroMemory(surfaceBuilder.isLoopEdge, edgeCapacity);
    memCopy(surfaceBuilder.gfxVertices, surface->gfxVertices, sizeof(Vtx) * surface->vertexCount);

    struct Vector2s16* prev = &loop[0];

    surfaceBuilder.edgeOnSearchLoop = portalSurfaceFindEnclosingFace(surface, prev);

    if (surfaceBuilder.edgeOnSearchLoop == -1) {
        goto error;
    }

    if (!portalSurfaceFindStartingPoint(&surfaceBuilder, prev)) {
        goto error;
    }

    for (int index = 1; index <= PORTAL_LOOP_SIZE;) {
        struct Vector2s16* next = &loop[index == PORTAL_LOOP_SIZE ? 0 : index];

        if (!portalSurfaceFindNextLoop(&surfaceBuilder, next)) {
            goto error;
        }

        struct Vector2s16* newPoint = portalSurfaceIntersectEdgeWithLoop(&surfaceBuilder, prev, next, index == PORTAL_LOOP_SIZE);

        if (!newPoint) {
            goto error;
        }

        // check if the portal loop ever intersected an edge
        if (index == PORTAL_LOOP_SIZE && !surfaceBuilder.hasConnected) {
            // the portal loop is fully contained
            int firstEdgeIndex = surface->edgeCount;
            int lastEdgeIndex = surfaceBuilder.currentEdge - 2;

            struct SurfaceEdge* firstEdge = portalSurfaceGetEdge(&surfaceBuilder, firstEdgeIndex);
            struct SurfaceEdge* lastEdge = portalSurfaceGetEdge(&surfaceBuilder, lastEdgeIndex);

            if (firstEdge->reverseEdge == NO_EDGE_CONNECTION || lastEdge->reverseEdge == NO_EDGE_CONNECTION) {
                goto error;
            }

            struct SurfaceEdge* firstEdgeReverse = portalSurfaceGetEdge(&surfaceBuilder, firstEdge->reverseEdge);
            struct SurfaceEdge* lastEdgeReverse = portalSurfaceGetEdge(&surfaceBuilder, lastEdge->reverseEdge);
            firstEdgeReverse->nextEdge = lastEdge->reverseEdge;
            lastEdgeReverse->prevEdge = firstEdge->reverseEdge;
            lastEdgeReverse->pointIndex = firstEdge->pointIndex;

            firstEdge->prevEdge = lastEdgeIndex;
            lastEdge->nextEdge = firstEdgeIndex;

            --surfaceBuilder.currentVertex;

            if (!portalSurfaceJoinInnerLoopToOuterLoop(&surfaceBuilder)) {
                goto error;
            }
        }

        if (newPoint->equalTest == next->equalTest) {
            // only increment i if the next point was reached
            ++index;
        }

        prev = newPoint;
    }

    portalSurfaceMarkHoleAsUsed(&surfaceBuilder);

    if (!portalSurfaceTriangulate(&surfaceBuilder)) {
        goto error;
    }

    result->vertices = malloc(sizeof(struct Vector2s16) * surfaceBuilder.currentVertex);
    result->edges = malloc(sizeof(struct SurfaceEdge) * surfaceBuilder.currentEdge);
    result->edgeCount = surfaceBuilder.currentEdge;
    result->vertexCount = surfaceBuilder.currentVertex;
    result->right = surface->right;
    result->up = surface->up;
    result->shouldCleanup = 1;
    result->corner = surface->corner;

    memCopy(result->vertices, surfaceBuilder.vertices, sizeof(struct Vector2s16) * surfaceBuilder.currentVertex);
    memCopy(result->edges, surfaceBuilder.edges, sizeof(struct SurfaceEdge) * surfaceBuilder.currentEdge);

    struct DisplayListResult displayList = newGfxFromSurfaceBuilder(&surfaceBuilder);

    result->gfxVertices = displayList.vtx;
    result->triangles = displayList.gfx;

    stackMallocFree(surfaceBuilder.edgeFlags);
    stackMallocFree(surfaceBuilder.isLoopEdge);
    stackMallocFree(surfaceBuilder.edges);
    stackMallocFree(surfaceBuilder.vertices);

    return 1;

error:
    stackMallocFree(surfaceBuilder.edgeFlags);
    stackMallocFree(surfaceBuilder.isLoopEdge);
    stackMallocFree(surfaceBuilder.edges);
    stackMallocFree(surfaceBuilder.vertices);
    return 0;
};