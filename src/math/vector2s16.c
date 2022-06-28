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