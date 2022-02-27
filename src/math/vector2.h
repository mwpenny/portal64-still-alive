#ifndef _MATH_VECTOR2_H
#define _MATH_VECTOR2_H

struct Vector2 {
    float x, y;
};

extern struct Vector2 gRight2;
extern struct Vector2 gUp2;
extern struct Vector2 gZeroVec2;

void vector2ComplexMul(struct Vector2* a, struct Vector2* b, struct Vector2* out);
void vector2ComplexConj(struct Vector2* a, struct Vector2* out);
void vector2RotateTowards(struct Vector2* from, struct Vector2* towards, struct Vector2* max, struct Vector2* out);
void vector2ComplexFromAngle(float radians, struct Vector2* out);
void vector2Rotate90(struct Vector2* input, struct Vector2* out);
float vector2Cross(struct Vector2* a, struct Vector2* b);
float vector2Dot(struct Vector2* a, struct Vector2* b);
float vector2MagSqr(struct Vector2* a);
float vector2DistSqr(struct Vector2* a, struct Vector2* b);
void vector2Add(struct Vector2* a, struct Vector2* b, struct Vector2* out);
void vector2Scale(struct Vector2* a, float scale, struct Vector2* out);
void vector2Normalize(struct Vector2* a, struct Vector2* out);
void vector2Sub(struct Vector2* a, struct Vector2* b, struct Vector2* out);
void vector2Negate(struct Vector2* a, struct Vector2* out);

void vector2Min(struct Vector2* a, struct Vector2* b, struct Vector2* out);
void vector2Max(struct Vector2* a, struct Vector2* b, struct Vector2* out);

#endif