#include "portal_surface_gfx.h"

#include "../util/memory.h"

#define GFX_VERTEX_CACHE_SIZE   32

void gfxBuilderFillTriangle(struct GfxBuilderState* gfxBuilder, struct PortalSurfaceBuilder* surfaceBuilder, int edge) {
    int nextEdge = portalSurfaceNextEdge(surfaceBuilder, edge);
    int prevEdge = portalSurfacePrevEdge(surfaceBuilder, edge);

    struct SurfaceEdge* edgePtr = portalSurfaceGetEdge(surfaceBuilder, edge);
    struct SurfaceEdge* nextEdgePtr = portalSurfaceGetEdge(surfaceBuilder, nextEdge);
    struct SurfaceEdge* prevEdgePtr = portalSurfaceGetEdge(surfaceBuilder, prevEdge);

    portalSurfaceSetFlag(surfaceBuilder, edge, SurfaceEdgeFlagsFilled);
    portalSurfaceSetFlag(surfaceBuilder, nextEdge, SurfaceEdgeFlagsFilled);
    portalSurfaceSetFlag(surfaceBuilder, prevEdge, SurfaceEdgeFlagsFilled);

    struct GfxTraingleIndices* triangle = &gfxBuilder->triangles[gfxBuilder->triangleCount];

    triangle->indices[0] = edgePtr->pointIndex;
    triangle->indices[1] = nextEdgePtr->pointIndex;
    triangle->indices[2] = prevEdgePtr->pointIndex;

    ++gfxBuilder->triangleCount;
}

void gfxBuilderCollectTriangles(struct GfxBuilderState* gfxBuilder, struct PortalSurfaceBuilder* surfaceBuilder) {
    for (int edge = 0; edge < surfaceBuilder->currentEdge; ++edge) {
        struct SurfaceEdge* edgePtr = portalSurfaceGetEdge(surfaceBuilder, edge);

        if (edgePtr->nextEdge == NO_EDGE_CONNECTION) {
            continue;
        }

        if (!portalSurfaceHasFlag(surfaceBuilder, edge, SurfaceEdgeFlagsFilled)) {
            gfxBuilderFillTriangle(gfxBuilder, surfaceBuilder, edge);
        }
    }
}

#define NO_VERTEX   0xFF

void sortU8Array(u8* input, u8* tmpBuffer, int min, int max) {
    int count = max - min;

    if (count <= 1) {
        return;
    } 
    
    if (count == 2) {
        if (input[min] > input[max - 1]) {
            int tmp = input[min];
            input[min] = input[max - 1];
            input[max - 1] = tmp;
        }

        return;
    }

    int midpoint = (min + max) >> 1;

    sortU8Array(input, tmpBuffer, min, midpoint);
    sortU8Array(input, tmpBuffer, midpoint, max);

    int aHead = min;
    int bHead = midpoint;
    int writeHead = min;

    while (aHead < midpoint && bHead < max) {
        if (input[aHead] < input[bHead]) {
            tmpBuffer[writeHead] = input[aHead];
            ++aHead;
            ++writeHead;
        } else {
            tmpBuffer[writeHead] = input[bHead];
            ++bHead;
            ++writeHead;
        }
    }

    while (aHead < midpoint) {
        tmpBuffer[writeHead] = input[aHead];
        ++aHead;
        ++writeHead;
    }

    while (bHead < max) {
        tmpBuffer[writeHead] = input[bHead];
        ++bHead;
        ++writeHead;
    }

    for (int i = min; i < max; ++i) {
        input[i] = tmpBuffer[i];
    }
}

void gfxBuilderFlushCache(struct GfxBuilderState* gfxBuilder, u8* vertexCacheContents, u8* vertexToCacheIndex, int startTriangle, int endTriangle, int usedSlotsCount) {
    u8 tmp[GFX_VERTEX_CACHE_SIZE];
    sortU8Array(vertexCacheContents, tmp, 0, usedSlotsCount);

    int lastStart = 0;

    for (int i = 0; i < usedSlotsCount; ++i) {
        // update the mapping
        vertexToCacheIndex[vertexCacheContents[i]] = i;

        // check if the vertex needs to updated
        if (i > 0 && vertexCacheContents[i - 1] + 1 != vertexCacheContents[i]) {
            gSPVertex(gfxBuilder->gfx++, &gfxBuilder->vtxCopy[vertexCacheContents[lastStart]], i - lastStart, lastStart);
            lastStart = i;
        }
    }

    if (usedSlotsCount == lastStart) {
        return;
    }

    gSPVertex(gfxBuilder->gfx++, &gfxBuilder->vtxCopy[vertexCacheContents[lastStart]], usedSlotsCount - lastStart, lastStart);

    for (int triangleIndex = startTriangle; triangleIndex < endTriangle; triangleIndex += 2) {
        struct GfxTraingleIndices* triangle = &gfxBuilder->triangles[triangleIndex];

        if (triangleIndex + 1 < endTriangle) {
            struct GfxTraingleIndices* nextTriangle = &gfxBuilder->triangles[triangleIndex + 1];

            gSP2Triangles(gfxBuilder->gfx++, 
                vertexToCacheIndex[triangle->indices[0]], vertexToCacheIndex[triangle->indices[1]], vertexToCacheIndex[triangle->indices[2]], 0, 
                vertexToCacheIndex[nextTriangle->indices[0]], vertexToCacheIndex[nextTriangle->indices[1]], vertexToCacheIndex[nextTriangle->indices[2]], 0
            );
        } else {
            gSP1Triangle(gfxBuilder->gfx++,
                vertexToCacheIndex[triangle->indices[0]], vertexToCacheIndex[triangle->indices[1]], vertexToCacheIndex[triangle->indices[2]], 0
            );
        }
    }
}

void gfxBuilderBuildGfx(struct GfxBuilderState* gfxBuilder, struct PortalSurfaceBuilder* surfaceBuilder) {
    u8 vertexCacheContents[GFX_VERTEX_CACHE_SIZE];
    u8* vertexToCacheIndex = stackMalloc(surfaceBuilder->currentVertex);
    int usedSlotsCount = 0;
    int triangleStart = 0;

    for (int i = 0; i < GFX_VERTEX_CACHE_SIZE; ++i) {
        vertexCacheContents[i] = NO_VERTEX;
    }

    for (int i = 0; i < surfaceBuilder->currentVertex; ++i) {
        vertexToCacheIndex[i] = NO_VERTEX;
    }

    for (int triangleIndex = 0; triangleIndex < gfxBuilder->triangleCount; ++triangleIndex) {
        struct GfxTraingleIndices* triangle = &gfxBuilder->triangles[triangleIndex];

        int neededSlots = 0;

        for (int index = 0; index < 3; ++index) {
            if (vertexToCacheIndex[triangle->indices[index]] == NO_VERTEX) {
                ++neededSlots;
            }
        }

        if (neededSlots + usedSlotsCount > GFX_VERTEX_CACHE_SIZE) {
            gfxBuilderFlushCache(gfxBuilder, vertexCacheContents, vertexToCacheIndex, triangleStart, triangleIndex, usedSlotsCount);
            neededSlots = 3;
            usedSlotsCount = 0;
            triangleStart = triangleIndex;

            for (int i = 0; i < GFX_VERTEX_CACHE_SIZE; ++i) {
                vertexCacheContents[i] = NO_VERTEX;
            }

            for (int i = 0; i < surfaceBuilder->currentVertex; ++i) {
                vertexToCacheIndex[i] = NO_VERTEX;
            }
        }

        for (int index = 0; index < 3; ++index) {
            int vertex = triangle->indices[index];
            if (vertexToCacheIndex[vertex] == NO_VERTEX) {
                vertexToCacheIndex[vertex] = usedSlotsCount;
                vertexCacheContents[usedSlotsCount] = vertex;
                ++usedSlotsCount;
            }
        }
    }

    gfxBuilderFlushCache(gfxBuilder, vertexCacheContents, vertexToCacheIndex, triangleStart, gfxBuilder->triangleCount, usedSlotsCount);

    gSPEndDisplayList(gfxBuilder->gfx++);

    stackMallocFree(vertexToCacheIndex);
}

struct DisplayListResult newGfxFromSurfaceBuilder(struct PortalSurfaceBuilder* surfaceBuilder) {
    struct GfxBuilderState builderState;

    int possibleTriangleCount = surfaceBuilder->currentEdge / 3;

    builderState.triangles = stackMalloc(sizeof(struct GfxTraingleIndices) * possibleTriangleCount);
    builderState.triangleCount = 0;

    builderState.vtxCopy = malloc(sizeof(Vtx) * surfaceBuilder->currentVertex);

    memCopy(builderState.vtxCopy, surfaceBuilder->gfxVertices, sizeof(Vtx) * surfaceBuilder->currentVertex);

    gfxBuilderCollectTriangles(&builderState, surfaceBuilder);

    Gfx* tmpResult = stackMalloc(builderState.triangleCount * sizeof(Gfx));
    builderState.gfx = tmpResult;

    gfxBuilderBuildGfx(&builderState, surfaceBuilder);

    int finalGfxSize = sizeof(Gfx) * (builderState.gfx - tmpResult);

    struct DisplayListResult result;

    result.gfx = malloc(finalGfxSize);
    result.vtx = builderState.vtxCopy;
    memCopy(result.gfx, tmpResult, finalGfxSize);

    stackMallocFree(tmpResult);
    stackMallocFree(builderState.triangles);

    return result;
}