#ifndef __MATRIX_H__
#define __MATRIX_H__

#include "vector4.h"
#include "vector3.h"

void matrixPerspective(float matrix[4][4], unsigned short* perspNorm, float l, float r, float top, float b, float near, float far);

float matrixNormalizedZValue(float depth, float nearPlane, float farPlane);

void matrixVec3Mul(float matrix[4][4], struct Vector3* input, struct Vector4* output);

void matrixFromBasis(float matrix[4][4], struct Vector3* origin, struct Vector3* x, struct Vector3* y, struct Vector3* z);

#endif