#include "Vector4.h"

Vector4::Vector4(): x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
Vector4::Vector4(float x, float y, float z, float w): x(x), y(y), z(z), w(w) {}

Vector4 operator * (const aiMatrix4x4& matrix, const Vector4& vector) {
    return Vector4(
        matrix.a1 * vector.x + matrix.a2 * vector.y + matrix.a3 * vector.z + matrix.a4 * vector.w,
        matrix.b1 * vector.x + matrix.b2 * vector.y + matrix.b3 * vector.z + matrix.b4 * vector.w,
        matrix.c1 * vector.x + matrix.c2 * vector.y + matrix.c3 * vector.z + matrix.c4 * vector.w,
        matrix.d1 * vector.x + matrix.d2 * vector.y + matrix.d3 * vector.z + matrix.d4 * vector.w
    );
}