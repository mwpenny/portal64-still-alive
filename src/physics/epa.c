#include "epa.h"

#include "../util/assert.h"
#include "../util/memory.h"

#define MAX_ITERATIONS  10

#define MAX_SIMPLEX_POINTS      (4 + MAX_ITERATIONS)
#define MAX_SIMPLEX_TRIANGLES   (4 + MAX_ITERATIONS * 2)

struct SimplexTriangle {
    unsigned char indices[3];
    float distanceToOrigin;
    struct Vector3 normal;
};

struct ExpandingSimplex {
    struct Vector3 points[MAX_SIMPLEX_POINTS];
    unsigned pointCount;
    struct SimplexTriangle triangles[MAX_SIMPLEX_TRIANGLES];
    unsigned triangleCount;
    unsigned char triangleHeap[MAX_SIMPLEX_TRIANGLES];
};

int expandingSimplexAddPoint(struct ExpandingSimplex* simplex, struct Vector3* point) {
    if (simplex->pointCount == MAX_SIMPLEX_POINTS) {
        return -1;
    }

    int result = simplex->pointCount;
    simplex->points[result] = *point;
    ++simplex->pointCount; 

    return result;
}

void expandingSimplexTriangleInit(struct ExpandingSimplex* simplex, int a, int b, int c, struct SimplexTriangle* triangle) {
    triangle->indices[0] = a;
    triangle->indices[1] = b;
    triangle->indices[2] = c;

    struct Vector3 edgeB;
    vector3Sub(&simplex->points[b], &simplex->points[a], &edgeB);
    struct Vector3 edgeC;
    vector3Sub(&simplex->points[c], &simplex->points[a], &edgeC);

    vector3Cross(&edgeB, &edgeC, &triangle->normal);
    vector3Normalize(&triangle->normal, &triangle->normal);
    triangle->distanceToOrigin = vector3Dot(&triangle->normal, &simplex->points[a]);
}

#define GET_PARENT_INDEX(heapIndex) (((heapIndex) - 1) >> 1)
#define GET_CHILD_INDEX(heapIndex, childHeapIndex)  (((heapIndex) << 1) + 1 + (childHeapIndex))
#define EXPANDING_SIMPLEX_GET_DISTANCE(simplex, triangleIndex)  ((simplex)->triangles[triangleIndex].distanceToOrigin)

void expandingSimplexSiftDownHeap(struct ExpandingSimplex* simplex, int heapIndex) {
    int parentHeapIndex = GET_PARENT_INDEX(heapIndex);
    float currentDistance = EXPANDING_SIMPLEX_GET_DISTANCE(simplex, simplex->triangleHeap[heapIndex]);

    while (heapIndex > 0) {
        // already heaped
        if (currentDistance >= EXPANDING_SIMPLEX_GET_DISTANCE(simplex, simplex->triangleHeap[parentHeapIndex])) {
            return;
        }

        // swap the parent with the current node
        int tmp = simplex->triangleHeap[heapIndex];
        simplex->triangleHeap[heapIndex] = simplex->triangleHeap[parentHeapIndex];
        simplex->triangleHeap[parentHeapIndex] = tmp;

        // move up to the parent
        heapIndex = parentHeapIndex;
        parentHeapIndex = GET_PARENT_INDEX(heapIndex);
    }
}

void expandingSimplexSiftUpHeap(struct ExpandingSimplex* simplex, int heapIndex) {
    float currentDistance = EXPANDING_SIMPLEX_GET_DISTANCE(simplex, simplex->triangleHeap[heapIndex]);

    while (heapIndex < simplex->triangleCount) {
        int swapWithChild;
        
        for (swapWithChild = 0; swapWithChild < 2; ++swapWithChild) {
            int childHeapIndex = GET_CHILD_INDEX(heapIndex, swapWithChild);

            // check that we don't run off the end of the heap
            if (childHeapIndex >= simplex->triangleCount) {
                break;
            }

            if (EXPANDING_SIMPLEX_GET_DISTANCE(simplex, simplex->triangleHeap[heapIndex]) < currentDistance) {
                swapWithChild = childHeapIndex;
                break;
            }
        }

        if (swapWithChild == 2) {
            // no child out of order
            return;
        }

        int childHeapIndex = GET_CHILD_INDEX(heapIndex, swapWithChild);

        // swap child with the current node
        int tmp = simplex->triangleHeap[heapIndex];
        simplex->triangleHeap[heapIndex] = simplex->triangleHeap[childHeapIndex];
        simplex->triangleHeap[childHeapIndex] = tmp;

        heapIndex = childHeapIndex;
    }
}

void expandingSimplexAddTriangle(struct ExpandingSimplex* simplex, int a, int b, int c) {
    if (simplex->triangleCount == MAX_SIMPLEX_TRIANGLES) {
        return;
    }

    int result = simplex->triangleCount;
    expandingSimplexTriangleInit(simplex, a, b, c, &simplex->triangles[result]);

    simplex->triangleHeap[result] = result;

    ++simplex->triangleCount;
    
    expandingSimplexSiftDownHeap(simplex, result);
}

void expandingSimplexReplaceFrontTriangle(struct ExpandingSimplex* simplex, int a, int b, int c) {
    int triangleIndex = simplex->triangleHeap[0];
    // resuse memory of front triangle
    expandingSimplexTriangleInit(simplex, a, b, c, &simplex->triangles[triangleIndex]);
    // ajust new position in heap
    expandingSimplexSiftUpHeap(simplex, 0);
}

struct SimplexTriangle* expandingSimplexClosestFace(struct ExpandingSimplex* simplex) {
    return &simplex->triangles[simplex->triangleHeap[0]];
}

void expandingSimplexInit(struct ExpandingSimplex* expandingSimplex, struct Simplex* simplex) {
    __assert(simplex->nPoints == 4);

    expandingSimplex->triangleCount = 0;
    expandingSimplex->pointCount = 0;

    expandingSimplexAddPoint(expandingSimplex, &simplex->points[0]);
    expandingSimplexAddPoint(expandingSimplex, &simplex->points[1]);
    expandingSimplexAddPoint(expandingSimplex, &simplex->points[2]);
    expandingSimplexAddPoint(expandingSimplex, &simplex->points[3]);

    expandingSimplexAddTriangle(expandingSimplex, 0, 1, 2);
    expandingSimplexAddTriangle(expandingSimplex, 2, 1, 3);
    expandingSimplexAddTriangle(expandingSimplex, 0, 2, 3);
    expandingSimplexAddTriangle(expandingSimplex, 1, 0, 3);
}

int expandingSimplexExpand(struct ExpandingSimplex* expandingSimplex, struct Vector3* newPoint) {
    int newPointIndex = expandingSimplexAddPoint(expandingSimplex, newPoint);

    if (newPointIndex == -1) {
        return 0;
    }

    struct SimplexTriangle* faceToRemove = expandingSimplexClosestFace(expandingSimplex);

    int existing0 = faceToRemove->indices[0];
    int existing1 = faceToRemove->indices[1];
    int existing2 = faceToRemove->indices[2];

    // replace the front triangle
    expandingSimplexReplaceFrontTriangle(expandingSimplex, existing2, existing0, newPointIndex);

    // add two new triangles
    expandingSimplexAddTriangle(expandingSimplex, existing0, existing1, newPointIndex);
    expandingSimplexAddTriangle(expandingSimplex, existing1, existing2, newPointIndex);

    return 1;
}

void epaSolve(struct Simplex* startingSimplex, void* data, MinkowsiSum sum, struct EpaResult* result) {
    struct ExpandingSimplex* simplex = stackMalloc(sizeof(struct ExpandingSimplex));
    expandingSimplexInit(simplex, startingSimplex);
    struct Vector3 nextPoint;
    struct SimplexTriangle* closestFace = NULL;
    float projection = 0.0f;

    int i;

    for (i = 0; i < MAX_ITERATIONS; ++i) {
        closestFace = expandingSimplexClosestFace(simplex);
        sum(data, &closestFace->normal, &nextPoint);

        projection = vector3Dot(&nextPoint, &closestFace->normal);

        if ((projection - closestFace->distanceToOrigin) < 0.00000001f) {
            break;
        }

        if (!expandingSimplexExpand(simplex, &nextPoint)) {
            break;
        }
    }

    if (closestFace) {
        result->normal = closestFace->normal;
        result->penetration = projection;
    }

    stackMallocFree(simplex);
}