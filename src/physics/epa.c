#include "epa.h"

#include "../util/assert.h"

#define MAX_SIMPLEX_POINTS      32
#define MAX_SIMPLEX_TRIANGLES   32

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
    ++simplex->pointCount;
    simplex->points[result] = *point;

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
    triangle->distanceToOrigin = -vector3Dot(&triangle->normal, &simplex->points[a]);
}


int expandingSimplexAddTriangle(struct ExpandingSimplex* simplex, int a, int b, int c) {
    if (simplex->triangleCount == MAX_SIMPLEX_TRIANGLES) {
        return -1;
    }

    int result = simplex->triangleCount;
    ++simplex->triangleCount;
    
    expandingSimplexTriangleInit(simplex, a, b, c, &simplex->triangles[result]);

    return result;
}

void expandingSimplexInit(struct ExpandingSimplex* expandingSimplex, struct Simplex* simplex) {
    __assert(simplex->nPoints == 4);

    expandingSimplexAddPoint(expandingSimplex, &simplex->points[0]);
    expandingSimplexAddPoint(expandingSimplex, &simplex->points[1]);
    expandingSimplexAddPoint(expandingSimplex, &simplex->points[2]);
    expandingSimplexAddPoint(expandingSimplex, &simplex->points[3]);

    expandingSimplexAddTriangle(simplex, 2, 1, 0);
    expandingSimplexAddTriangle(simplex, 1, 2, 3);
    expandingSimplexAddTriangle(simplex, 2, 0, 3);
    expandingSimplexAddTriangle(simplex, 0, 1, 3);
}

void epaSolve(struct Simplex* startingSimplex, void* data, MinkowsiSum sum) {

}