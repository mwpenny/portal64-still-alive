
#ifndef _QUATERNION_H
#define _QUATERNION_H

#include "vector3.h"
#include "vector2.h"

struct Quaternion {
    float x, y, z, w;
};

extern struct Quaternion gQuaternionZero;

void quatIdent(struct Quaternion* q);
void quatAxisAngle(struct Vector3* axis, float angle, struct Quaternion* out);
void quatAxisComplex(struct Vector3* axis, struct Vector2* complex, struct Quaternion* out);
void quatConjugate(struct Quaternion* in, struct Quaternion* out);
void quatNegate(struct Quaternion* in, struct Quaternion* out);
void quatMultVector(struct Quaternion* q, struct Vector3* a, struct Vector3* out);
void quatRotatedBoundingBoxSize(struct Quaternion* q, struct Vector3* halfBoxSize, struct Vector3* out);
void quatMultiply(struct Quaternion* a, struct Quaternion* b, struct Quaternion* out);
void quatAdd(struct Quaternion* a, struct Quaternion* b, struct Quaternion* out);
void quatToMatrix(struct Quaternion* q, float out[4][4]);
void quatNormalize(struct Quaternion* q, struct Quaternion* out);
void quatRandom(struct Quaternion* q);
void quatLook(struct Vector3* lookDir, struct Vector3* up, struct Quaternion* out);
void quatEulerAngles(struct Vector3* angles, struct Quaternion* out);
// cheap approximation of slerp
void quatLerp(struct Quaternion* a, struct Quaternion* b, float t, struct Quaternion* out);
void quatApplyAngularVelocity(struct Quaternion* input, struct Vector3* w, float timeStep, struct Quaternion* output);
void quatDecompose(struct Quaternion* input, struct Vector3* axis, float* angle);

float quatDot(struct Quaternion* a, struct Quaternion* b);

#endif