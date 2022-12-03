
#include "transform.h"
#include <assert.h>

void transformInitIdentity(struct Transform* in) {
    in->position = gZeroVec;
    quatIdent(&in->rotation);
    in->scale = gOneVec;
}

void transformToMatrix(struct Transform* in, float mtx[4][4], float sceneScale) {
    quatToMatrix(&in->rotation, mtx);

    mtx[0][0] *= in->scale.x; mtx[0][1] *= in->scale.x; mtx[0][2] *= in->scale.x;
    mtx[1][0] *= in->scale.y; mtx[1][1] *= in->scale.y; mtx[1][2] *= in->scale.y;
    mtx[2][0] *= in->scale.z; mtx[2][1] *= in->scale.z; mtx[2][2] *= in->scale.z;

    mtx[3][0] = in->position.x * sceneScale;
    mtx[3][1] = in->position.y * sceneScale;
    mtx[3][2] = in->position.z * sceneScale;
}


void transformToMatrixL(struct Transform* in, Mtx* mtx, float sceneScale) {
    float mtxf[4][4];
    transformToMatrix(in, mtxf, sceneScale);
    guMtxF2L(mtxf, mtx);
}

void transformInvert(struct Transform* in, struct Transform* out) {
    assert(in != out);

    float uniformScale = 1.0f;

    if (in->scale.x != 1.0f || in->scale.y != 1.0f || in->scale.z != 1.0f) {
        uniformScale = 3.0f / (in->scale.x + in->scale.y + in->scale.z);
    }

    // order of transforms forward scale -> rotate -> translate
    // order of transforms inverse translate^-1 -> rotate^-1 -> scale^-1

    if (uniformScale == 1.0f) {
        out->scale.x = in->scale.x;
        out->scale.y = in->scale.y;
        out->scale.z = in->scale.z;
    } else {
        out->scale.x = 1.0f / in->scale.x;
        out->scale.y = 1.0f / in->scale.y;
        out->scale.z = 1.0f / in->scale.z;
    }

    quatConjugate(&in->rotation, &out->rotation);
    vector3Negate(&in->position, &out->position);
    quatMultVector(&out->rotation, &out->position, &out->position);

    if (uniformScale != 1.0f) {
        vector3Scale(&out->position, &out->position, 1.0f / uniformScale);
    }
}

void transformPoint(struct Transform* transform, struct Vector3* in, struct Vector3* out) {
    vector3Multiply(&transform->scale, in, out);
    quatMultVector(&transform->rotation, out, out);
    vector3Add(&transform->position, out, out);
}

void transformPointInverse(struct Transform* transform, struct Vector3* in, struct Vector3* out) {
    vector3Sub(in, &transform->position, out);
    struct Quaternion quatInverse;
    quatConjugate(&transform->rotation, &quatInverse);
    quatMultVector(&quatInverse, out, out);
    out->x /= transform->scale.x;
    out->y /= transform->scale.y;
    out->z /= transform->scale.z;
}

void transformPointInverseNoScale(struct Transform* transform, struct Vector3* in, struct Vector3* out) {
    vector3Sub(in, &transform->position, out);
    struct Quaternion quatInverse;
    quatConjugate(&transform->rotation, &quatInverse);
    quatMultVector(&quatInverse, out, out);
}

void transformConcat(struct Transform* left, struct Transform* right, struct Transform* output) {
    vector3Multiply(&left->scale, &right->scale, &output->scale);
    struct Vector3 rotatedOffset;
    quatMultVector(&left->rotation, &right->position, &rotatedOffset);
    quatMultiply(&left->rotation, &right->rotation, &output->rotation);

    output->position.x = left->position.x + rotatedOffset.x * left->scale.x;
    output->position.y = left->position.y + rotatedOffset.y * left->scale.y;
    output->position.z = left->position.z + rotatedOffset.z * left->scale.z;
}

void transformLerp(struct Transform* a, struct Transform* b, float t, struct Transform* output) {
    vector3Lerp(&a->position, &b->position, t, &output->position);
    quatLerp(&a->rotation, &b->rotation, t, &output->rotation);
    vector3Lerp(&a->scale, &b->scale, t, &output->scale);
}