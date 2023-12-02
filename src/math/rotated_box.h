#ifndef __MATH_ROTATED_BOX_H__
#define __MATH_ROTATED_BOX_H__

#include "vector3.h"
#include "./boxs16.h"
#include "./transform.h"

struct RotatedBox {
    struct Vector3 origin;
    struct Vector3 sides[3];
};

void rotatedBoxTransform(struct Transform* transform, struct BoundingBoxs16* input, struct RotatedBox* output);

#endif