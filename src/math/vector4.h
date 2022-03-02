
#ifndef _VECTOR4_H
#define _VECTOR4_H

struct Vector4 {
    float x, y, z, w;
};

void vector4Lerp(struct Vector4* a, struct Vector4* b, float lerp, struct Vector4* out);

#endif