#ifndef __MATH_VECTOR2S16_H__
#define __MATH_VECTOR2S16_H__

#include "vector3.h"

struct Vector2s16 {
    union {
        struct {
            short x;
            short y;
        };
        int equalTest;
    };
};

#define VECTOR2s16_AS_ARRAY(vector) ((s16*)(vector))

void vector2s16Add(struct Vector2s16* a, struct Vector2s16* b, struct Vector2s16* output);
void vector2s16Sub(struct Vector2s16* a, struct Vector2s16* b, struct Vector2s16* output);

int vector2s16Dot(struct Vector2s16* a, struct Vector2s16* b);
int vector2s16Cross(struct Vector2s16* a, struct Vector2s16* b);
int vector2s16MagSqr(struct Vector2s16* a);
int vector2s16DistSqr(struct Vector2s16* a, struct Vector2s16* b);

int vector2s16FallsBetween(struct Vector2s16* from, struct Vector2s16* towards, struct Vector2s16* check);

void vector2s16Barycentric(struct Vector2s16* a, struct Vector2s16* b, struct Vector2s16* c, struct Vector2s16* point, struct Vector3* output);

#endif