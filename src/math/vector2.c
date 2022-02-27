
#include "vector2.h"
#include "mathf.h"

struct Vector2 gRight2 = {1.0f, 0.0f};
struct Vector2 gUp2 = {0.0f, 1.0f};
struct Vector2 gZeroVec2 = {0.0f, 0.0f};

void vector2ComplexMul(struct Vector2* a, struct Vector2* b, struct Vector2* out) {
    float x = a->x * b->x - a->y * b->y;
    out->y = a->x * b->y + a->y * b->x;
    out->x = x;
}

void vector2ComplexConj(struct Vector2* a, struct Vector2* out) {
    out->x = a->x;
    out->y = -a->y;
}

void vector2ComplexFromAngle(float radians, struct Vector2* out) {
    out->x = cosf(radians);
    out->y = sinf(radians);
}

void vector2RotateTowards(struct Vector2* from, struct Vector2* towards, struct Vector2* max, struct Vector2* out) {
    struct Vector2 fromInv = {from->x, -from->y};
    struct Vector2 diff;
    vector2ComplexMul(&fromInv, towards, &diff);

    if (diff.x > max->x) {
        *out = *towards;
    } else {
        if (diff.y < 0) {
            diff.x = max->x;
            diff.y = -max->y;
        } else {
            diff = *max;
        }
        vector2ComplexMul(from, &diff, out);
    }
}

void vector2Rotate90(struct Vector2* input, struct Vector2* out) {
    // in case input == out
    float tmp = input->x;
    out->x = -input->y;
    out->y = tmp;
}

float vector2Cross(struct Vector2* a, struct Vector2* b) {
    return a->x * b->y - a->y * b->x;
}

float vector2Dot(struct Vector2* a, struct Vector2* b) {
    return a->x * b->x + a->y * b->y;
}

void vector2Add(struct Vector2* a, struct Vector2* b, struct Vector2* out) {
    out->x = a->x + b->x;
    out->y = a->y + b->y;
}

void vector2Sub(struct Vector2* a, struct Vector2* b, struct Vector2* out) {
    out->x = a->x - b->x;
    out->y = a->y - b->y;
}

void vector2Scale(struct Vector2* a, float scale, struct Vector2* out) {
    out->x = a->x * scale;
    out->y = a->y * scale;
}

float vector2MagSqr(struct Vector2* a)  {
    return a->x * a->x + a->y * a->y;
}

float vector2DistSqr(struct Vector2* a, struct Vector2* b) {
    float dx = a->x - b->x;
    float dy = a->y - b->y;

    return dx * dx + dy * dy;
}

void vector2Normalize(struct Vector2* a, struct Vector2* out) {
    if (a->x == 0.0f && a->y == 0.0f) {
        *out = *a;
        return;
    }

    float denom = sqrtf(a->x * a->x + a->y * a->y);

    if (denom < 0.0000001f) {
        *out = *a;
        return;
    }

    float scale = 1.0f / denom;
    out->x = a->x * scale;
    out->y = a->y * scale;
}

void vector2Negate(struct Vector2* a, struct Vector2* out) {
    out->x = -a->x;
    out->y = -a->y;
}

void vector2Min(struct Vector2* a, struct Vector2* b, struct Vector2* out) {
    out->x = minf(a->x, b->x);
    out->y = minf(a->y, b->y);
}

void vector2Max(struct Vector2* a, struct Vector2* b, struct Vector2* out) {
    out->x = maxf(a->x, b->x);
    out->y = maxf(a->y, b->y);
}