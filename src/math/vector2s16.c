#include "vector2s16.h"

void vector2s16Add(struct Vector2s16* a, struct Vector2s16* b, struct Vector2s16* output) {
    output->x = a->x + b->x;
    output->y = a->y + b->y;
}

void vector2s16Sub(struct Vector2s16* a, struct Vector2s16* b, struct Vector2s16* output) {
    output->x = a->x - b->x;
    output->y = a->y - b->y;
}

int vector2s16Dot(struct Vector2s16* a, struct Vector2s16* b) {
    return (int)a->x * b->x + (int)a->y * b->y;
}

int vector2s16Cross(struct Vector2s16* a, struct Vector2s16* b) {
    return (int)a->x * b->y - (int)a->y * b->x;
}

int vector2s16MagSqr(struct Vector2s16* a) {
    return (int)a->x * a->x + (int)a->y * a->y;
}

int vector2s16DistSqr(struct Vector2s16* a, struct Vector2s16* b) {
    int x = (int)a->x - (int)b->x;
    int y = (int)a->y - (int)b->y;

    return x * x + y * y;
}

int vector2s16FallsBetween(struct Vector2s16* from, struct Vector2s16* towards, struct Vector2s16* check) {
    int directionCross = vector2s16Cross(from, towards);

    if (directionCross == 0) {
        return vector2s16Cross(from, check) >= 0;
    } else if (directionCross > 0) {
        return vector2s16Cross(from, check) >= 0 && vector2s16Cross(check, towards) >= 0;
    } else {
        return vector2s16Cross(from, check) >= 0 || vector2s16Cross(check, towards) >= 0;
    }
}

void vector2s16Barycentric(struct Vector2s16* a, struct Vector2s16* b, struct Vector2s16* c, struct Vector2s16* point, struct Vector3* output) {
    struct Vector2s16 v0;
    struct Vector2s16 v1;
    struct Vector2s16 v2;

    vector2s16Sub(b, a, &v0);
    vector2s16Sub(c, a, &v1);
    vector2s16Sub(point, a, &v2);

    float d00 = (float)vector2s16Dot(&v0, &v0);
    float d01 = (float)vector2s16Dot(&v0, &v1);
    float d11 = (float)vector2s16Dot(&v1, &v1);
    float d20 = (float)vector2s16Dot(&v2, &v0);
    float d21 = (float)vector2s16Dot(&v2, &v1);

    float denom = 1.0f / (d00 * d11 - d01 * d01);
    output->y = (d11 * d20 - d01 * d21) * denom;
    output->z = (d00 * d21 - d01 * d20) * denom;
    output->x = 1.0f - output->y - output->z;
}