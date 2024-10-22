#ifndef __VECTOR4_H__
#define __VECTOR4_H__

#include <assimp/matrix4x4.h>

struct Vector4 {
    Vector4();
    Vector4(float x, float y, float z, float w);
    float x;
    float y;
    float z;
    float w;
};

Vector4 operator * (const aiMatrix4x4& matrix, const Vector4& vector);

#endif