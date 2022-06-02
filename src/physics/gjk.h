#ifndef __GJK_H___
#define __GJK_H___

#include "../math/vector3.h"

typedef int (*MinkowsiSum)(void* data, struct Vector3* direction, struct Vector3* output);

#define MAX_SIMPLEX_SIZE    4

#define COMBINE_CONTACT_IDS(a, b)   ((((a) & 0xFFFF) << 16) | ((b) & 0xFFFF))

struct Simplex {
    struct Vector3 points[MAX_SIMPLEX_SIZE];
    struct Vector3 objectAPoint[MAX_SIMPLEX_SIZE];
    int ids[MAX_SIMPLEX_SIZE];
    short nPoints;
};

void simplexInit(struct Simplex* simplex);
int simplexCheck(struct Simplex* simplex, struct Vector3* nextDirection);

int gjkCheckForOverlap(struct Simplex* simplex, void* objectA, MinkowsiSum objectASum, void* objectB, MinkowsiSum objectBSum, struct Vector3* firstDirection);

#endif