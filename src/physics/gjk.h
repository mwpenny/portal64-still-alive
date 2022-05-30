#ifndef __GJK_H___
#define __GJK_H___

#include "../math/vector3.h"

typedef void (*MinkowsiSum)(void* data, struct Vector3* direction, struct Vector3* output);

#define MAX_SIMPLEX_SIZE    4

struct Simplex {
    struct Vector3 points[MAX_SIMPLEX_SIZE];
    short nPoints;
};

void simplexInit(struct Simplex* simplex, struct Vector3* firstPoint);
int simplexAddPoint(struct Simplex* simplex, struct Vector3* point);
int simplexCheck(struct Simplex* simplex, struct Vector3* nextDirection);

int gjkCheckForOverlap(struct Simplex* simplex, void* data, MinkowsiSum sum, struct Vector3* firstDirection);

#endif