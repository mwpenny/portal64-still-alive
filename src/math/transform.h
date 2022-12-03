#ifndef _TRANSFORM_H
#define _TRANSFORM_H

#include <ultra64.h>
#include "./vector3.h"
#include "./quaternion.h"

struct Transform {
    struct Vector3 position;
    struct Quaternion rotation;
    struct Vector3 scale;
};

void transformInitIdentity(struct Transform* in);
void transformToMatrix(struct Transform* in, float mtx[4][4], float sceneScale);
void transformToMatrixL(struct Transform* in, Mtx* mtx, float sceneScale);
void transformInvert(struct Transform* in, struct Transform* out);
void transformPoint(struct Transform* transform, struct Vector3* in, struct Vector3* out);
void transformPointInverse(struct Transform* transform, struct Vector3* in, struct Vector3* out);
void transformPointInverseNoScale(struct Transform* transform, struct Vector3* in, struct Vector3* out);
void transformConcat(struct Transform* left, struct Transform* right, struct Transform* output);

void transformLerp(struct Transform* a, struct Transform* b, float t, struct Transform* output);

#endif