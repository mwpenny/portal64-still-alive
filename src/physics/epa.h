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

void epaSolve(struct Simplex* startingSimplex, void* objectA, MinkowsiSum objectASum, void* objectB, MinkowsiSum objectBSum, struct EpaResult* result);
void epaSwapResult(struct EpaResult* result);

#endif