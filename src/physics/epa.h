#ifndef __EPA_H__
#define __EPA_H__

#include "gjk.h"

struct EpaResult {
    struct Vector3 contactA;
    struct Vector3 contactB;
    // points from A to B
    struct Vector3 normal;
    float penetration;
    int id;
};

void epaSolve(struct Simplex* startingSimplex, void* objectA, MinkowskiSupport objectASupport, void* objectB, MinkowskiSupport objectBSupport, struct EpaResult* result);
int epaSolveSwept(struct Simplex* startingSimplex, void* objectA, MinkowskiSupport objectASupport, void* objectB, MinkowskiSupport objectBSupport, struct Vector3* bStart, struct Vector3* bEnd, struct EpaResult* result);
void epaSwapResult(struct EpaResult* result);

#endif