#include "portal_surface_generator.h"

#include "portal.h"
#include "../util/memory.h"
#include "../math/mathf.h"
#include "../math/vector2.h"
#include "portal_surface_gfx.h"

#define IS_ORIGINAL_VERTEX_INDEX(surfaceBuilder, vertexIndex) ((vertexIndex) < (surfaceBuilder)->original->vertexCount)

#define MAX_SEARCH_ITERATIONS   32

#define ADDITIONAL_EDGE_CAPACITY 64
#define ADDITIONAL_VERTEX_CAPACITY 32

#define VERIFY_INTEGRITY    0

#define VERY_FAR_AWAY   1e15f

int portalSurfaceFindEnclosingFace(struct PortalSurface* surface, struct Vector2s16* aroundPoint, struct SurfaceEdgeWithSide* output) {
    float minDistance = VERY_FAR_AWAY;

    struct Vector2 aroundPointF;

    aroundPointF.x = (float)aroundPoint->x;
    aroundPointF.y = (float)aroundPoint->y;

    for (int i = 0; i < surface->edgeCount; ++i) {
        struct SurfaceEdge* edge = &surface->edges[i];

        struct Vector2s16* as16 = &surface->vertices[edge->aIndex];
        struct Vector2s16* bs16 = &surface->vertices[edge->bIndex];

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

            output->edgeIndex = i;
            output->isReverse = vector2Cross(&edgeOffset, &pointOffset) < 0.0f;
        }
    }

    return minDistance != VERY_FAR_AWAY;
}

struct SurfaceEdge* portalSurfaceGetEdge(struct PortalSurfaceBuilder* surfaceBuilder, int edgeIndex) {
    return &surfaceBuilder->edges[edgeIndex];
}

struct Vector2s16* portalSurfaceGetVertex(struct PortalSurfaceBuilder* surfaceBuilder, int vertexIndex) {
    return &surfaceBuilder->vertices[vertexIndex];
}

void portalSurfaceNextEdge(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* currentEdge, struct SurfaceEdgeWithSide* nextEdge) {
    struct SurfaceEdge* edge = portalSurfaceGetEdge(surfaceBuilder, currentEdge->edgeIndex);
    int edgeIndex = currentEdge->edgeIndex;

    nextEdge->edgeIndex = SB_GET_NEXT_EDGE(edge, currentEdge->isReverse);
    nextEdge->isReverse = portalSurfaceGetEdge(surfaceBuilder, nextEdge->edgeIndex)->prevEdgeReverse == edgeIndex;
}

void portalSurfacePrevEdge(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* currentEdge, struct SurfaceEdgeWithSide* prevEdge) {
    struct SurfaceEdge* edge = portalSurfaceGetEdge(surfaceBuilder, currentEdge->edgeIndex);
    int edgeIndex = currentEdge->edgeIndex;

    prevEdge->edgeIndex = SB_GET_PREV_EDGE(edge, currentEdge->isReverse);
    prevEdge->isReverse = portalSurfaceGetEdge(surfaceBuilder, prevEdge->edgeIndex)->nextEdgeReverse == edgeIndex;
}

#if VERIFY_INTEGRITY

int portalSurfaceIsWellFormed(struct PortalSurfaceBuilder* surfaceBuilder) {
    for (int i = 0; i < surfaceBuilder->currentEdge; ++i) {
        struct SurfaceEdgeWithSide current;
        current.edgeIndex = i;
        
        for (current.isReverse = 0; current.isReverse < 2; ++current.isReverse) {
            if (current.isReverse && portalSurfaceGetEdge(surfaceBuilder, i)->nextEdgeReverse == NO_EDGE_CONNECTION) {
                break;
            }

            struct SurfaceEdgeWithSide check;
            portalSurfaceNextEdge(surfaceBuilder, &current, &check);
            struct SurfaceEdge* checkPtr = portalSurfaceGetEdge(surfaceBuilder, check.edgeIndex);

            if (checkPtr->prevEdge != i && checkPtr->prevEdgeReverse != i) {
                return 0;
            }

            portalSurfacePrevEdge(surfaceBuilder, &current, &check);
            checkPtr = portalSurfaceGetEdge(surfaceBuilder, check.edgeIndex);

            if (checkPtr->nextEdge != i && checkPtr->nextEdgeReverse != i) {
                return 0;
            }
        }
    }

    return 1;
}

#endif

int portalSurfacePointInsideFace(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* currentEdge, struct Vector2s16* point) {
    struct SurfaceEdgeWithSide nextEdge;
    portalSurfaceNextEdge(surfaceBuilder, currentEdge, &nextEdge);

    struct SurfaceEdge* edgePtr = portalSurfaceGetEdge(surfaceBuilder, currentEdge->edgeIndex);
    struct SurfaceEdge* nextEdgePtr = portalSurfaceGetEdge(surfaceBuilder, nextEdge.edgeIndex);

    struct Vector2s16* corner = portalSurfaceGetVertex(surfaceBuilder, SB_GET_NEXT_POINT(edgePtr, currentEdge->isReverse));
    struct Vector2s16* prevPoint = portalSurfaceGetVertex(surfaceBuilder, SB_GET_CURRENT_POINT(edgePtr, currentEdge->isReverse));
    struct Vector2s16* nextPoint = portalSurfaceGetVertex(surfaceBuilder, SB_GET_NEXT_POINT(nextEdgePtr, nextEdge.isReverse));

    struct Vector2s16 nextDir;
    struct Vector2s16 prevDir;

    struct Vector2s16 pointDir;

    vector2s16Sub(nextPoint, corner, &nextDir);
    vector2s16Sub(prevPoint, corner, &prevDir);
    vector2s16Sub(point, corner, &pointDir);

    return vector2s16FallsBetween(&nextDir, &prevDir, &pointDir);
}

int portalSurfaceIsEdgeValid(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* currentEdge) {
    return SB_GET_NEXT_EDGE(portalSurfaceGetEdge(surfaceBuilder, currentEdge->edgeIndex), currentEdge->isReverse) != NO_EDGE_CONNECTION;
}

int portalSurfaceNextLoop(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* currentEdge, struct SurfaceEdgeWithSide* nextFace) {
    portalSurfaceNextEdge(surfaceBuilder, currentEdge, nextFace);
    nextFace->isReverse = !nextFace->isReverse;

    if (portalSurfaceIsEdgeValid(surfaceBuilder, nextFace)) {
        return 1;
    }

    *nextFace = *currentEdge;
    nextFace->isReverse = !nextFace->isReverse;

    int i = 0;

    while (i < MAX_SEARCH_ITERATIONS && portalSurfaceIsEdgeValid(surfaceBuilder, nextFace)) {
        struct SurfaceEdgeWithSide prevFace;
        portalSurfacePrevEdge(surfaceBuilder, nextFace, &prevFace);

        *nextFace = prevFace;
        nextFace->isReverse = !nextFace->isReverse;
        ++i;
    }

    nextFace->isReverse = !nextFace->isReverse;

    return i < MAX_SEARCH_ITERATIONS;
}

int portalSurfaceFindCurrentFace(struct PortalSurfaceBuilder* surfaceBuilder, struct Vector2s16* forPoint, struct SurfaceEdgeWithSide* edgeToAdjust) {
    int i = 0;

    while (i < MAX_SEARCH_ITERATIONS && !portalSurfacePointInsideFace(surfaceBuilder, edgeToAdjust, forPoint)) {
        struct SurfaceEdgeWithSide nextEdge;

        if (!portalSurfaceNextLoop(surfaceBuilder, edgeToAdjust, &nextEdge)) {
            return 0;
        }

        if (i != 0 && edgeToAdjust->edgeIndex == surfaceBuilder->edgeOnSearchLoop.edgeIndex) {
            // a full loop and no face found
            return 0;
        }

        *edgeToAdjust = nextEdge;
        ++i;
    }

    return i < MAX_SEARCH_ITERATIONS;
}

int portalSurfaceFindNextLoop(struct PortalSurfaceBuilder* surfaceBuilder, struct Vector2s16* forPoint) {
    if (!surfaceBuilder->hasConnected) {
        return 1;
    }

    struct SurfaceEdgeWithSide currentEdge = surfaceBuilder->edgeOnSearchLoop;

    int nextIndex = SB_GET_NEXT_EDGE(portalSurfaceGetEdge(surfaceBuilder, surfaceBuilder->cuttingEdge.edgeIndex), surfaceBuilder->cuttingEdge.isReverse);

    // the cutting edge is its own next edge
    // this means edgeOnSearchLoop will already be correct
    if (nextIndex == surfaceBuilder->cuttingEdge.edgeIndex) {
        return 1;
    }

    if (!portalSurfaceFindCurrentFace(surfaceBuilder, forPoint, &currentEdge)) {
        return 0;
    }

    surfaceBuilder->edgeOnSearchLoop = currentEdge;
    surfaceBuilder->cuttingEdge = currentEdge;

    return 1;
}

#define MAX_INTERSECT_LOOPS 20

int portalSurfaceIsPointOnLine(struct Vector2s16* pointA, struct Vector2s16* edgeA, struct Vector2s16* edgeDir) {
    struct Vector2s16 originOffset;
    struct Vector2s16 endpointOffset;

    if (edgeDir->equalTest == 0) {
        return 0;
    }

    vector2s16Add(edgeA, edgeDir, &endpointOffset);
    vector2s16Sub(&endpointOffset, pointA, &endpointOffset);

    vector2s16Sub(edgeA, pointA, &originOffset);

    int crossProduct = vector2s16Cross(&originOffset, &endpointOffset);
    int edgeDirLength = vector2s16MagSqr(edgeDir);

    s64 angleCheck = (s64)crossProduct * 50LL / (s64)edgeDirLength;
    return angleCheck == 0;
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

    if (denominator > 0 ? (edgeLerp < 0 || edgeLerp > denominator) : (edgeLerp > 0 || edgeLerp < denominator)) {
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

int portalSurfaceNewEdge(struct PortalSurfaceBuilder* surfaceBuilder, int isLoopEdge, struct OriginalEdgeMapping* originalEdge) {
    while (surfaceBuilder->checkForEdgeReuse < surfaceBuilder->currentEdge) {
        struct SurfaceEdge* edge = portalSurfaceGetEdge(surfaceBuilder, surfaceBuilder->checkForEdgeReuse);

        if (edge->nextEdge == NO_EDGE_CONNECTION && edge->nextEdgeReverse == NO_EDGE_CONNECTION) {
            surfaceBuilder->isLoopEdge[surfaceBuilder->checkForEdgeReuse] = isLoopEdge;
            surfaceBuilder->originalEdgeIndex[surfaceBuilder->checkForEdgeReuse] = *originalEdge;

            return surfaceBuilder->checkForEdgeReuse;
        }

        ++surfaceBuilder->checkForEdgeReuse;
    }

    if (surfaceBuilder->currentEdge == ADDITIONAL_EDGE_CAPACITY + surfaceBuilder->original->edgeCount) {
        return -1;
    }

    int newEdgeIndex = surfaceBuilder->currentEdge;
    surfaceBuilder->isLoopEdge[surfaceBuilder->currentEdge] = isLoopEdge;
    surfaceBuilder->originalEdgeIndex[newEdgeIndex] = *originalEdge;
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

void portalSurfaceCalcVertex(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* loopEdge, int resultIndex) {
    struct SurfaceEdgeWithSide originalEdge;
    struct OriginalEdgeMapping originalEdgeMapping = surfaceBuilder->originalEdgeIndex[loopEdge->edgeIndex];
    SB_ORIGINAL_EDGE_TO_EDGE_WITH_SIDE(loopEdge->isReverse ? originalEdgeMapping.originalEdgeReverse : originalEdgeMapping.originalEdge, &originalEdge);

    struct SurfaceEdge* originalEdgePtr = &surfaceBuilder->original->edges[originalEdge.edgeIndex];
    struct SurfaceEdgeWithSide nextEdge;
    nextEdge.edgeIndex = SB_GET_NEXT_EDGE(originalEdgePtr, originalEdge.isReverse);

    struct SurfaceEdge* nextEdgePtr = &surfaceBuilder->original->edges[nextEdge.edgeIndex];
    nextEdge.isReverse = nextEdgePtr->prevEdgeReverse == originalEdge.edgeIndex;

    int aIndex = SB_GET_CURRENT_POINT(originalEdgePtr, originalEdge.isReverse);
    int bIndex = SB_GET_NEXT_POINT(originalEdgePtr, originalEdge.isReverse);
    int cIndex = SB_GET_NEXT_POINT(nextEdgePtr, nextEdge.isReverse);

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

int portalSurfaceSplitEdge(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* edge, struct Vector2s16* point) {
    int newVertexIndex = portalSurfaceNewVertex(surfaceBuilder, point);
    
    if (newVertexIndex == -1) {
        return -1;
    }
    
    struct SurfaceEdge* existingEdge = portalSurfaceGetEdge(surfaceBuilder, edge->edgeIndex);

    struct SurfaceEdgeWithSide nextEdge;
    struct SurfaceEdgeWithSide prevReverseEdge;

    int hasReverseEdge = existingEdge->nextEdgeReverse != NO_EDGE_CONNECTION;

    portalSurfaceNextEdge(surfaceBuilder, edge, &nextEdge);

    if (hasReverseEdge) {
        prevReverseEdge = *edge;
        prevReverseEdge.isReverse = !prevReverseEdge.isReverse;
        portalSurfacePrevEdge(surfaceBuilder, &prevReverseEdge, &prevReverseEdge);
    }

    int newEdgeIndex = portalSurfaceNewEdge(surfaceBuilder, 0, &surfaceBuilder->originalEdgeIndex[edge->edgeIndex]);

    if (newEdgeIndex == -1) {
        return -1;
    }

    portalSurfaceLerpVtx(surfaceBuilder, SB_GET_CURRENT_POINT(existingEdge, edge->isReverse), SB_GET_NEXT_POINT(existingEdge, edge->isReverse), newVertexIndex);

    struct SurfaceEdge* newEdge = portalSurfaceGetEdge(surfaceBuilder, newEdgeIndex);

    SB_SET_NEXT_POINT(newEdge, edge->isReverse, SB_GET_NEXT_POINT(existingEdge, edge->isReverse));
    SB_SET_CURRENT_POINT(newEdge, edge->isReverse, newVertexIndex);

    SB_SET_NEXT_EDGE(newEdge, edge->isReverse, SB_GET_NEXT_EDGE(existingEdge, edge->isReverse));
    SB_SET_PREV_EDGE(newEdge, edge->isReverse, edge->edgeIndex);
    SB_SET_NEXT_EDGE(newEdge, !edge->isReverse, hasReverseEdge ? edge->edgeIndex : NO_EDGE_CONNECTION);
    SB_SET_PREV_EDGE(newEdge, !edge->isReverse, hasReverseEdge ? SB_GET_PREV_EDGE(existingEdge, !edge->isReverse) : NO_EDGE_CONNECTION);

    SB_SET_NEXT_POINT(existingEdge, edge->isReverse, newVertexIndex);

    SB_SET_NEXT_EDGE(existingEdge, edge->isReverse, newEdgeIndex);
    if (hasReverseEdge) {
        SB_SET_PREV_EDGE(existingEdge, !edge->isReverse, newEdgeIndex);
    }

    struct SurfaceEdge* nextEdgePtr = portalSurfaceGetEdge(surfaceBuilder, nextEdge.edgeIndex);

    SB_SET_PREV_EDGE(nextEdgePtr, nextEdge.isReverse, newEdgeIndex);

    if (hasReverseEdge) {
        struct SurfaceEdge* prevEdgePtr = portalSurfaceGetEdge(surfaceBuilder, prevReverseEdge.edgeIndex);
        SB_SET_NEXT_EDGE(prevEdgePtr, prevReverseEdge.isReverse, newEdgeIndex);
    }

#if VERIFY_INTEGRITY
    if (!portalSurfaceIsWellFormed(surfaceBuilder)) {
        return -1;
    }
#endif

    return newVertexIndex;
}

int portalSurfaceConnectToPoint(struct PortalSurfaceBuilder* surfaceBuilder, int pointIndex, struct SurfaceEdgeWithSide* edge, int isLoopEdge) {
    struct OriginalEdgeMapping originalEdge = surfaceBuilder->originalEdgeIndex[surfaceBuilder->edgeOnSearchLoop.edgeIndex];

    if (surfaceBuilder->edgeOnSearchLoop.isReverse) {
        originalEdge.originalEdge = originalEdge.originalEdgeReverse;
    } else {
        originalEdge.originalEdgeReverse = originalEdge.originalEdge;
    }

    int newEdge = portalSurfaceNewEdge(surfaceBuilder, isLoopEdge, &originalEdge);

    if (newEdge == -1) {
        return 0;
    }

    struct SurfaceEdge* newEdgePtr = portalSurfaceGetEdge(surfaceBuilder, newEdge);

    if (surfaceBuilder->hasEdge) {
        struct SurfaceEdgeWithSide nextEdge;
        portalSurfaceNextEdge(surfaceBuilder, &surfaceBuilder->cuttingEdge, &nextEdge);

        struct SurfaceEdge* cuttingEdgePtr = portalSurfaceGetEdge(surfaceBuilder, surfaceBuilder->cuttingEdge.edgeIndex);
        struct SurfaceEdge* nextEdgePtr = portalSurfaceGetEdge(surfaceBuilder, nextEdge.edgeIndex);

        newEdgePtr->aIndex = SB_GET_NEXT_POINT(cuttingEdgePtr, surfaceBuilder->cuttingEdge.isReverse);

        newEdgePtr->prevEdge = surfaceBuilder->cuttingEdge.edgeIndex;
        newEdgePtr->nextEdgeReverse = nextEdge.edgeIndex;

        SB_SET_NEXT_EDGE(cuttingEdgePtr, surfaceBuilder->cuttingEdge.isReverse, newEdge);
        SB_SET_PREV_EDGE(nextEdgePtr, nextEdge.isReverse, newEdge);
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

        SB_SET_NEXT_EDGE(edgePtr, edge->isReverse, newEdge);
        SB_SET_PREV_EDGE(nextEdgePtr, nextEdge.isReverse, newEdge);
    } else {
        newEdgePtr->nextEdge = newEdge;
        newEdgePtr->prevEdgeReverse = newEdge;
    }

    surfaceBuilder->hasEdge = 1;
    surfaceBuilder->cuttingEdge.edgeIndex = newEdge;
    surfaceBuilder->cuttingEdge.isReverse = 0;

#if VERIFY_INTEGRITY
    if (!portalSurfaceIsWellFormed(surfaceBuilder)) {
        return -1;
    }
#endif

    return 1;
}

#define COLLAPSE_DISTANCE   8

struct Vector2s16* portalSurfaceIntersectEdgeWithLoop(struct PortalSurfaceBuilder* surfaceBuilder, struct Vector2s16* pointA, struct Vector2s16* pointB, int isFinalPoint) {
    struct SurfaceEdgeWithSide currentEdge = surfaceBuilder->edgeOnSearchLoop;

    struct Vector2s16 pointDir;
    vector2s16Sub(pointB, pointA, &pointDir);

    int iteration;

    for (iteration = 0; 
        iteration < MAX_INTERSECT_LOOPS && (
            iteration == 0 || 
            currentEdge.edgeIndex != surfaceBuilder->edgeOnSearchLoop.edgeIndex || 
            currentEdge.isReverse != surfaceBuilder->edgeOnSearchLoop.isReverse
        ); portalSurfaceNextEdge(surfaceBuilder, &currentEdge, &currentEdge), ++iteration) {
            
        if (!isFinalPoint && surfaceBuilder->isLoopEdge[currentEdge.edgeIndex]) {
            continue;
        }

        struct SurfaceEdge* edge = portalSurfaceGetEdge(surfaceBuilder, currentEdge.edgeIndex);

        struct Vector2s16* edgeA = portalSurfaceGetVertex(surfaceBuilder, SB_GET_CURRENT_POINT(edge, currentEdge.isReverse));
        struct Vector2s16* edgeB = portalSurfaceGetVertex(surfaceBuilder, SB_GET_NEXT_POINT(edge, currentEdge.isReverse));

        struct Vector2s16 intersectionPoint;

        enum IntersectionType intersectType = portalSurfaceIntersect(pointA, &pointDir, edgeA, edgeB, &intersectionPoint);

        if (intersectionPoint.equalTest == pointA->equalTest) {
            continue;
        }

        if (intersectType == IntersectionTypePoint) {
            int newPointIndex;

            if (vector2s16DistSqr(&intersectionPoint, pointA) <= COLLAPSE_DISTANCE * COLLAPSE_DISTANCE) {
                intersectionPoint = *pointA;
            }

            if (vector2s16DistSqr(&intersectionPoint, pointB) <= COLLAPSE_DISTANCE * COLLAPSE_DISTANCE) {
                intersectionPoint = *pointB;
            }

            if (intersectionPoint.equalTest == edgeA->equalTest) {
                newPointIndex = SB_GET_CURRENT_POINT(edge, currentEdge.isReverse);
                portalSurfaceFindCurrentFace(surfaceBuilder, pointA, &currentEdge);
            } else if (intersectionPoint.equalTest == edgeB->equalTest) {
                newPointIndex = SB_GET_NEXT_POINT(edge, currentEdge.isReverse);
                portalSurfaceFindCurrentFace(surfaceBuilder, pointA, &currentEdge);
            } else {
                newPointIndex = portalSurfaceSplitEdge(surfaceBuilder, &currentEdge, &intersectionPoint);
                
                if (newPointIndex == -1) {
                    return NULL;
                }
            }

            if (!portalSurfaceConnectToPoint(surfaceBuilder, newPointIndex, &currentEdge, 1)) {
                return NULL;
            }

            surfaceBuilder->hasConnected = 1;
            surfaceBuilder->edgeOnSearchLoop = surfaceBuilder->cuttingEdge;

            return portalSurfaceGetVertex(surfaceBuilder, newPointIndex);
        } else if (intersectType == IntersectionTypeColinear) {
            int newPointIndex;

            if (intersectionPoint.equalTest == edgeA->equalTest) {
                newPointIndex = SB_GET_CURRENT_POINT(edge, currentEdge.isReverse);
                portalSurfacePrevEdge(surfaceBuilder, &currentEdge, &surfaceBuilder->cuttingEdge);
            } else if (intersectionPoint.equalTest == edgeB->equalTest) {
                newPointIndex = SB_GET_NEXT_POINT(edge, currentEdge.isReverse);
                surfaceBuilder->cuttingEdge = currentEdge;
            } else {
                newPointIndex = portalSurfaceSplitEdge(surfaceBuilder, &currentEdge, &intersectionPoint);

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

    portalSurfaceCalcVertex(surfaceBuilder, &surfaceBuilder->edgeOnSearchLoop, newPointIndex);

    if (!portalSurfaceConnectToPoint(surfaceBuilder, newPointIndex, NULL, 1)) {
        return NULL;
    }

    return portalSurfaceGetVertex(surfaceBuilder, newPointIndex);
}

int portalSurfaceFindStartingPoint(struct PortalSurfaceBuilder* surfaceBuilder, struct Vector2s16* point) {
    struct SurfaceEdgeWithSide currentEdge = surfaceBuilder->edgeOnSearchLoop;

    struct Vector2s16* edgeA = portalSurfaceGetVertex(surfaceBuilder, SB_GET_CURRENT_POINT(portalSurfaceGetEdge(surfaceBuilder, currentEdge.edgeIndex), currentEdge.isReverse));

    for (int iteration = 0; iteration < MAX_INTERSECT_LOOPS; ++iteration) {
        if (iteration > 0 && currentEdge.edgeIndex == surfaceBuilder->edgeOnSearchLoop.edgeIndex && currentEdge.isReverse == surfaceBuilder->edgeOnSearchLoop.isReverse) {
            // finished searching loop
            break;
        }

        struct Vector2s16* edgeB = portalSurfaceGetVertex(surfaceBuilder, SB_GET_NEXT_POINT(portalSurfaceGetEdge(surfaceBuilder, currentEdge.edgeIndex), currentEdge.isReverse));

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
        struct SurfaceEdgeWithSide nextEdge;
        portalSurfaceNextEdge(surfaceBuilder, &currentEdge, &nextEdge);
        currentEdge = nextEdge;
    }

    surfaceBuilder->hasEdge = 0;
    surfaceBuilder->hasConnected = 0;
    surfaceBuilder->startingPoint = portalSurfaceNewVertex(surfaceBuilder, point);

    portalSurfaceCalcVertex(surfaceBuilder, &surfaceBuilder->edgeOnSearchLoop, surfaceBuilder->startingPoint);

    return 1;
}

int portalSurfaceJoinInnerLoopToOuterLoop(struct PortalSurfaceBuilder* surfaceBuilder) {
    struct SurfaceEdge* outerLoopEdge = portalSurfaceGetEdge(surfaceBuilder, surfaceBuilder->edgeOnSearchLoop.edgeIndex);
    struct Vector2s16* outerLoopPoint = portalSurfaceGetVertex(surfaceBuilder, SB_GET_NEXT_POINT(outerLoopEdge, surfaceBuilder->edgeOnSearchLoop.isReverse));
    
    struct SurfaceEdgeWithSide currentEdge = surfaceBuilder->cuttingEdge;
    struct SurfaceEdgeWithSide nextEdge;

    struct SurfaceEdgeWithSide closestEdge;
    int closestDistance = 0x7FFFFFFF;

    while (portalSurfaceNextEdge(surfaceBuilder, &currentEdge, &nextEdge), nextEdge.edgeIndex != surfaceBuilder->cuttingEdge.edgeIndex) {
        struct Vector2s16* edgePoint = portalSurfaceGetVertex(
            surfaceBuilder, 
            SB_GET_NEXT_POINT(portalSurfaceGetEdge(surfaceBuilder, currentEdge.edgeIndex), currentEdge.isReverse)
        );

        int edgeDistance = vector2s16DistSqr(outerLoopPoint, edgePoint);

        if (edgeDistance < closestDistance) {
            closestDistance = edgeDistance;
            closestEdge = currentEdge;
        }

        currentEdge = nextEdge;
    }

    surfaceBuilder->cuttingEdge = closestEdge;
    return portalSurfaceConnectToPoint(surfaceBuilder, SB_GET_NEXT_POINT(outerLoopEdge, surfaceBuilder->edgeOnSearchLoop.isReverse), &surfaceBuilder->edgeOnSearchLoop, 0);
}

int portalSurfaceHasFlag(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* edge, enum SurfaceEdgeFlags value) {
    return ((surfaceBuilder->edgeFlags[edge->edgeIndex] & (edge->isReverse ? value << 4 : value))) != 0;
}

void portalSurfaceSetFlag(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* edge, enum SurfaceEdgeFlags value) {
    surfaceBuilder->edgeFlags[edge->edgeIndex] |= edge->isReverse ? value << 4 : value;
}

void portalSurfaceMarkLoopAsUsed(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* edgeOnLoop) {
    struct SurfaceEdgeWithSide currentEdge = *edgeOnLoop;

    int iteration;

    for (iteration = 0; iteration < MAX_SEARCH_ITERATIONS && (iteration == 0 || currentEdge.edgeIndex != edgeOnLoop->edgeIndex || currentEdge.isReverse != edgeOnLoop->isReverse); ++iteration) {
        struct SurfaceEdgeWithSide nextEdge;
        portalSurfaceNextEdge(surfaceBuilder, &currentEdge, &nextEdge);

        portalSurfaceSetFlag(surfaceBuilder, &currentEdge, SurfaceEdgeFlagsUsed);

        if (currentEdge.isReverse) {
            surfaceBuilder->edges[currentEdge.edgeIndex].nextEdgeReverse = NO_EDGE_CONNECTION;
            surfaceBuilder->edges[currentEdge.edgeIndex].prevEdgeReverse = NO_EDGE_CONNECTION;
        } else {
            surfaceBuilder->edges[currentEdge.edgeIndex].nextEdge = NO_EDGE_CONNECTION;
            surfaceBuilder->edges[currentEdge.edgeIndex].prevEdge = NO_EDGE_CONNECTION;
        }
        
        struct SurfaceEdge* surfaceEdge = portalSurfaceGetEdge(surfaceBuilder, nextEdge.edgeIndex);

        // since this function clears out nextEdge and prevEdge
        // portalSurfaceNextEdge will fail after the loop as been
        // marked. If this check fails the loop is finished
        if (surfaceEdge->prevEdge != currentEdge.edgeIndex && surfaceEdge->prevEdgeReverse != currentEdge.edgeIndex) {
            return;
        }

        currentEdge = nextEdge;
    }
}

void portalSurfaceMarkHoleAsUsed(struct PortalSurfaceBuilder* surfaceBuilder) {
    for (int i = 0; i < surfaceBuilder->currentEdge; ++i) {
        if (surfaceBuilder->isLoopEdge[i]) {
            struct SurfaceEdgeWithSide edgeOnLoop;
            edgeOnLoop.edgeIndex = i;
            edgeOnLoop.isReverse = 1;

            if (!portalSurfaceHasFlag(surfaceBuilder, &edgeOnLoop, SurfaceEdgeFlagsUsed)) {
                portalSurfaceMarkLoopAsUsed(surfaceBuilder, &edgeOnLoop);
            }
        }
    }

    // setting this causes the algorithm to reuse edges when running the
    // triangulate algorithm
    surfaceBuilder->checkForEdgeReuse = 0;
}

int portalSurfaceConnectEdges(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* from, struct SurfaceEdgeWithSide* to, struct SurfaceEdgeWithSide* newEdge) {
    struct SurfaceEdge* nextEdgePtr = portalSurfaceGetEdge(surfaceBuilder, to->edgeIndex);
    surfaceBuilder->cuttingEdge = *from;
    surfaceBuilder->hasEdge = 1;
    if (!portalSurfaceConnectToPoint(surfaceBuilder, SB_GET_NEXT_POINT(nextEdgePtr, to->isReverse), to, 0)) {
        return 0;
    }
    *newEdge = surfaceBuilder->cuttingEdge;
    return 1;
}

int portalSurfaceTriangulateLoop(struct PortalSurfaceBuilder* surfaceBuilder, struct SurfaceEdgeWithSide* edgeOnLoop) {
    if (portalSurfaceHasFlag(surfaceBuilder, edgeOnLoop, SurfaceEdgeFlagsTriangulated)) {
        return 1;
    }

    struct SurfaceEdgeWithSide currentEdge = *edgeOnLoop;
    int iteration = 0;
    struct SurfaceEdge* currentEdgePtr = portalSurfaceGetEdge(surfaceBuilder, currentEdge.edgeIndex);

    // Find the starting point
    while (!IS_ORIGINAL_VERTEX_INDEX(surfaceBuilder, SB_GET_CURRENT_POINT(currentEdgePtr, currentEdge.isReverse)) || IS_ORIGINAL_VERTEX_INDEX(surfaceBuilder, SB_GET_NEXT_POINT(currentEdgePtr, currentEdge.isReverse))) {
        ++iteration;
        portalSurfaceNextEdge(surfaceBuilder, &currentEdge, &currentEdge);
        currentEdgePtr = portalSurfaceGetEdge(surfaceBuilder, currentEdge.edgeIndex);

        if (iteration == MAX_SEARCH_ITERATIONS || (currentEdge.edgeIndex == edgeOnLoop->edgeIndex && currentEdge.isReverse == edgeOnLoop->isReverse)) {
            if (iteration == 3) {
                // already a triangle
                struct SurfaceEdgeWithSide nextEdge;
                struct SurfaceEdgeWithSide prevEdge;

                portalSurfaceNextEdge(surfaceBuilder, &currentEdge, &nextEdge);
                portalSurfacePrevEdge(surfaceBuilder, &currentEdge, &prevEdge);
                
                portalSurfaceSetFlag(surfaceBuilder, &currentEdge, SurfaceEdgeFlagsTriangulated);
                portalSurfaceSetFlag(surfaceBuilder, &nextEdge, SurfaceEdgeFlagsTriangulated);
                portalSurfaceSetFlag(surfaceBuilder, &prevEdge, SurfaceEdgeFlagsTriangulated);
                return 1;

            }

            return 0;
        }
    }

    for (iteration = 0; iteration < MAX_SEARCH_ITERATIONS; ++iteration) {
        currentEdgePtr = portalSurfaceGetEdge(surfaceBuilder, currentEdge.edgeIndex);

        struct Vector2s16* edgePointA = portalSurfaceGetVertex(surfaceBuilder, SB_GET_CURRENT_POINT(currentEdgePtr, currentEdge.isReverse));
        struct Vector2s16* edgePointB = portalSurfaceGetVertex(surfaceBuilder, SB_GET_NEXT_POINT(currentEdgePtr, currentEdge.isReverse));

        struct SurfaceEdgeWithSide nextEdge;
        struct SurfaceEdgeWithSide prevEdge;

        portalSurfaceNextEdge(surfaceBuilder, &currentEdge, &nextEdge);
        portalSurfacePrevEdge(surfaceBuilder, &currentEdge, &prevEdge);

        int prevPointIndex = SB_GET_CURRENT_POINT(portalSurfaceGetEdge(surfaceBuilder, prevEdge.edgeIndex), prevEdge.isReverse);

        struct Vector2s16* nextPoint = portalSurfaceGetVertex(surfaceBuilder, SB_GET_NEXT_POINT(portalSurfaceGetEdge(surfaceBuilder, nextEdge.edgeIndex), nextEdge.isReverse));
        struct Vector2s16* prevPoint = portalSurfaceGetVertex(surfaceBuilder, prevPointIndex);

        // check if finished
            if (nextPoint == prevPoint) {
            portalSurfaceSetFlag(surfaceBuilder, &currentEdge, SurfaceEdgeFlagsTriangulated);
            portalSurfaceSetFlag(surfaceBuilder, &nextEdge, SurfaceEdgeFlagsTriangulated);
            portalSurfaceSetFlag(surfaceBuilder, &prevEdge, SurfaceEdgeFlagsTriangulated);
            return 1;
        }

        struct Vector2s16 edgeDir;
        struct Vector2s16 nextEdgeDir;
        struct Vector2s16 prevEdgeDir;

        vector2s16Sub(edgePointB, edgePointA, &edgeDir);
        vector2s16Sub(nextPoint, edgePointB, &nextEdgeDir);
        vector2s16Sub(edgePointB, prevPoint, &prevEdgeDir);

        struct SurfaceEdgeWithSide nextCurrentEdge;
        if (IS_ORIGINAL_VERTEX_INDEX(surfaceBuilder, prevPointIndex) && vector2s16Cross(&prevEdgeDir, &nextEdgeDir) > vector2s16Cross(&edgeDir, &nextEdgeDir)) {
            struct SurfaceEdgeWithSide prevPrevEdge;
            portalSurfacePrevEdge(surfaceBuilder, &prevEdge, &prevPrevEdge);

            if (!portalSurfaceConnectEdges(surfaceBuilder, &prevPrevEdge, &currentEdge, &nextCurrentEdge)) {
                return 0;
            }

            portalSurfaceSetFlag(surfaceBuilder, &prevEdge, SurfaceEdgeFlagsTriangulated);
            portalSurfaceSetFlag(surfaceBuilder, &currentEdge, SurfaceEdgeFlagsTriangulated);
        } else {
            if (!portalSurfaceConnectEdges(surfaceBuilder, &prevEdge, &nextEdge, &nextCurrentEdge)) {
                return 0;
            }

            portalSurfaceSetFlag(surfaceBuilder, &currentEdge, SurfaceEdgeFlagsTriangulated);
            portalSurfaceSetFlag(surfaceBuilder, &nextEdge, SurfaceEdgeFlagsTriangulated);
        }

        currentEdge = nextCurrentEdge;

        nextCurrentEdge.isReverse = !nextCurrentEdge.isReverse;
        portalSurfaceSetFlag(surfaceBuilder, &nextCurrentEdge, SurfaceEdgeFlagsTriangulated);
    }

    return 0;
}

int portalSurfaceTriangulate(struct PortalSurfaceBuilder* surfaceBuilder) {
    struct SurfaceEdgeWithSide edge;

    for (int i = 0; i < surfaceBuilder->currentEdge; ++i) {
        edge.edgeIndex = i;
        
        for (edge.isReverse = 0; edge.isReverse < 2; ++edge.isReverse) {
            if (SB_GET_NEXT_EDGE(portalSurfaceGetEdge(surfaceBuilder, edge.edgeIndex), edge.isReverse) == NO_EDGE_CONNECTION) {
                continue;
            }

            if (!portalSurfaceTriangulateLoop(surfaceBuilder, &edge)) {
                return 0;
            }
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
    surfaceBuilder.originalEdgeIndex = stackMalloc(sizeof(struct OriginalEdgeMapping) * edgeCapacity);

    for (int i = 0; i < surface->edgeCount; ++i) {
        surfaceBuilder.originalEdgeIndex[i].originalEdge = i;
        surfaceBuilder.originalEdgeIndex[i].originalEdgeReverse = i | 0x80;
    }

    surfaceBuilder.gfxVertices = stackMalloc(sizeof(Vtx) * (surface->vertexCount + ADDITIONAL_EDGE_CAPACITY));

    zeroMemory(surfaceBuilder.edgeFlags, surface->edgeCount + edgeCapacity);
    zeroMemory(surfaceBuilder.isLoopEdge, surface->edgeCount + edgeCapacity);
    memCopy(surfaceBuilder.gfxVertices, surface->gfxVertices, sizeof(Vtx) * surface->vertexCount);

    struct Vector2s16* prev = &loop[0];

    if (!portalSurfaceFindEnclosingFace(surface, prev, &surfaceBuilder.edgeOnSearchLoop)) {
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
            int lastEdgeIndex = surfaceBuilder.currentEdge - 1;

            struct SurfaceEdge* firstEdge = portalSurfaceGetEdge(&surfaceBuilder, firstEdgeIndex);
            struct SurfaceEdge* lastEdge = portalSurfaceGetEdge(&surfaceBuilder, lastEdgeIndex);

            firstEdge->prevEdge = lastEdgeIndex;
            firstEdge->nextEdgeReverse = lastEdgeIndex;

            lastEdge->nextEdge = firstEdgeIndex;
            lastEdge->prevEdgeReverse = firstEdgeIndex;

            lastEdge->bIndex = firstEdge->aIndex;

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