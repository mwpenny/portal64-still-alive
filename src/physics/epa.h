#ifndef __EPA_H__
#define __EPA_H__

#include "gjk.h"

struct EpaResult {
    struct Vector3 normal;
    float penetration;
};

void epaSolve(struct Simplex* startingSimplex, void* data, MinkowsiSum sum, struct EpaResult* result);

#endif