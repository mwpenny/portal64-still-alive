#ifndef _MATH_BOX3D_H
#define _MATH_BOX3D_H

#include "vector3.h"

struct Box3D {
    struct Vector3 min;
    struct Vector3 max;
};

#endif