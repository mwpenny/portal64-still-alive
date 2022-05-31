#ifndef __BASIS_H__
#define __BASIS_H__

#include "vector3.h"
#include "quaternion.h"

struct Basis {
    struct Vector3 x;
    struct Vector3 y;
    struct Vector3 z;
};

void basisFromQuat(struct Basis* basis, struct Quaternion* quat);

void basisRotate(struct Basis* basis, struct Vector3* input, struct Vector3* output);
void basisUnRotate(struct Basis* basis, struct Vector3* input, struct Vector3* output);

#endif