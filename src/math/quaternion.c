
#include "quaternion.h"
#include <ultra64.h>
#include <assert.h>
#include "mathf.h"
#include <math.h>

struct Quaternion gQuaternionZero = {0.0f, 0.0f, 0.0f, 0.0f};

void quatIdent(struct Quaternion* q) {
    q->x = 0.0f;
    q->y = 0.0f;
    q->z = 0.0f;
    q->w = 1.0f;
}

void quatAxisAngle(struct Vector3* axis, float angle, struct Quaternion* out) {
    float sinTheta = sinf(angle * 0.5f);
    float cosTheta = cosf(angle * 0.5f);

    out->x = axis->x * sinTheta;
    out->y = axis->y * sinTheta;
    out->z = axis->z * sinTheta;
    out->w = cosTheta;
}

void quatEulerAngles(struct Vector3* angles, struct Quaternion* out) {
    struct Quaternion angle;
    struct Quaternion tmp;

    quatAxisAngle(&gRight, angles->x, &angle);
    quatAxisAngle(&gUp, angles->y, out);
    quatMultiply(out, &angle, &tmp);
    quatAxisAngle(&gForward, angles->z, &angle);
    quatMultiply(&angle, &tmp, out);
}

void quatAxisComplex(struct Vector3* axis, struct Vector2* complex, struct Quaternion* out) {
    float sinTheta = 0.5f - complex->x * 0.5f;

    if (sinTheta < 0.0f) {
        sinTheta = 0.0f;
    } else {
        sinTheta = sqrtf(sinTheta);

        if (complex->y < 0.0f) {
            sinTheta = -sinTheta;
        }
    }

    float cosTheta = 0.5f + complex->x * 0.5f;

    if (cosTheta < 0.0f) {
        cosTheta = 0.0f;
    } else {
        cosTheta = sqrtf(cosTheta);
    }

    out->x = axis->x * sinTheta;
    out->y = axis->y * sinTheta;
    out->z = axis->z * sinTheta;
    out->w = cosTheta;
}

void quatConjugate(struct Quaternion* in, struct Quaternion* out) {
    out->x = -in->x;
    out->y = -in->y;
    out->z = -in->z;
    out->w = in->w;
}

void quatNegate(struct Quaternion* in, struct Quaternion* out) {
    out->x = -in->x;
    out->y = -in->y;
    out->z = -in->z;
    out->w = -in->w;
}

void quatMultVector(struct Quaternion* q, struct Vector3* a, struct Vector3* out) {
    struct Quaternion tmp;
    struct Quaternion asQuat;
    struct Quaternion conj;
    asQuat.x = a->x;
    asQuat.y = a->y;
    asQuat.z = a->z;
    asQuat.w = 0.0f;

    quatMultiply(q, &asQuat, &tmp);
    quatConjugate(q, &conj);
    quatMultiply(&tmp, &conj, &asQuat);
    
    out->x = asQuat.x;
    out->y = asQuat.y;
    out->z = asQuat.z;
}

void quatRotatedBoundingBoxSize(struct Quaternion* q, struct Vector3* halfBoxSize, struct Vector3* out) {
    float xx = q->x*q->x;
    float yy = q->y*q->y;
    float zz = q->z*q->z;

    float xy = q->x*q->y;
    float yz = q->y*q->z;
    float xz = q->x*q->z;

    float xw = q->x*q->w;
    float yw = q->y*q->w;
    float zw = q->z*q->w;

    out->x = fabsf(1.0f - 2.0f * (yy + zz)) * halfBoxSize->x +
        fabsf(2.0f * (xy - zw)) * halfBoxSize->y +
        fabsf(2.0f * (xz + yw)) * halfBoxSize->z;

    out->y = fabsf(2.0f * (xy + zw)) * halfBoxSize->x +
        fabsf(1.0f - 2.0f * (xx + zz)) * halfBoxSize->y +
        fabsf(2.0f * (yz - xw)) * halfBoxSize->z;

    out->z = fabsf(2.0f * (xz - yw)) * halfBoxSize->x +
        fabsf(2.0f * (yz + xw)) * halfBoxSize->y +
        fabsf(1.0f - 2.0f * (xx + yy));
}

void quatMultiply(struct Quaternion* a, struct Quaternion* b, struct Quaternion* out) {
    assert(a != out && b != out);
    out->x = a->w*b->x + a->x*b->w + a->y*b->z - a->z*b->y;
    out->y = a->w*b->y + a->y*b->w + a->z*b->x - a->x*b->z;
    out->z = a->w*b->z + a->z*b->w + a->x*b->y - a->y*b->x;
    out->w = a->w*b->w - a->x*b->x - a->y*b->y - a->z*b->z;
}

void quatAdd(struct Quaternion* a, struct Quaternion* b, struct Quaternion* out) {
    out->x = a->x + b->x;
    out->y = a->y + b->y;
    out->z = a->z + b->z;
    out->w = a->w + b->w;
}

void quatToMatrix(struct Quaternion* q, float out[4][4]) {
    float xx = q->x*q->x;
    float yy = q->y*q->y;
    float zz = q->z*q->z;

    float xy = q->x*q->y;
    float yz = q->y*q->z;
    float xz = q->x*q->z;

    float xw = q->x*q->w;
    float yw = q->y*q->w;
    float zw = q->z*q->w;

    out[0][0] = 1.0f - 2.0f * (yy + zz);
    out[0][1] = 2.0f * (xy + zw);
    out[0][2] = 2.0f * (xz - yw);
    out[0][3] = 0.0f;
    out[1][0] = 2.0f * (xy - zw);
    out[1][1] = 1.0f - 2.0f * (xx + zz);
    out[1][2] = 2.0f * (yz + xw);
    out[1][3] = 0.0f;
    out[2][0] = 2.0f * (xz + yw);
    out[2][1] = 2.0f * (yz - xw);
    out[2][2] = 1.0f - 2.0f * (xx + yy);
    out[2][3] = 0.0f;
    out[3][0] = 0.0f;
    out[3][1] = 0.0f;
    out[3][2] = 0.0f;
    out[3][3] = 1.0f;
}

void quatNormalize(struct Quaternion* q, struct Quaternion* out) {
    float magSqr = q->x * q->x + q->y * q->y + q->z * q->z + q->w * q->w;

    if (magSqr < 0.00001f) {
        out->w = 1.0f;
        out->x = 0.0f;
        out->y = 0.0f;
        out->z = 0.0f;
    } else {
        magSqr = 1.0f / sqrtf(magSqr);

        out->x = q->x * magSqr;
        out->y = q->y * magSqr;
        out->z = q->z * magSqr;
        out->w = q->w * magSqr;
    }
}

void quatRandom(struct Quaternion* q) {
    q->x = mathfRandomFloat() - 0.5f;
    q->y = mathfRandomFloat() - 0.5f;
    q->z = mathfRandomFloat() - 0.5f;
    q->w = mathfRandomFloat() - 0.5f;
    quatNormalize(q, q);
}

void quatLook(struct Vector3* lookDir, struct Vector3* up, struct Quaternion* out) {
    // calculate orthonormal basis
    struct Vector3 zDir;
    vector3Normalize(lookDir, &zDir);
    vector3Negate(&zDir, &zDir);

    struct Vector3 yDir;
    vector3AddScaled(up, &zDir, -vector3Dot(&zDir, up), &yDir);
    vector3Normalize(&yDir, &yDir);

    struct Vector3 xDir;
    vector3Cross(&yDir, &zDir, &xDir);

    // convert orthonormal basis to a quaternion
    float trace = xDir.x + yDir.y + zDir.z;
    if (trace > 0) { 
        float sqrtResult = sqrtf(trace+1.0f) * 2.0f;
        float invSqrtResult = 1.0f / sqrtResult;
        out->w = 0.25 * sqrtResult;
        out->x = (yDir.z - zDir.y) * invSqrtResult;
        out->y = (zDir.x - xDir.z) * invSqrtResult; 
        out->z = (xDir.y - yDir.x) * invSqrtResult; 
    } else if ((xDir.x > yDir.y) && (xDir.x > zDir.z)) { 
        float sqrtResult = sqrtf(1.0 + xDir.x - yDir.y - zDir.z) * 2.0f;
        float invSqrtResult = 1.0f / sqrtResult;
        out->w = (yDir.z - zDir.y) * invSqrtResult;
        out->x = 0.25 * sqrtResult;
        out->y = (yDir.x + xDir.y) * invSqrtResult; 
        out->z = (zDir.x + xDir.z) * invSqrtResult; 
    } else if (yDir.y > zDir.z) { 
        float sqrtResult = sqrtf(1.0 + yDir.y - xDir.x - zDir.z) * 2.0f;
        float invSqrtResult = 1.0f / sqrtResult;
        out->w = (zDir.x - xDir.z) * invSqrtResult;
        out->x = (yDir.x + xDir.y) * invSqrtResult; 
        out->y = 0.25 * sqrtResult;
        out->z = (zDir.y + yDir.z) * invSqrtResult; 
    } else { 
        float sqrtResult = sqrtf(1.0 + zDir.z - xDir.x - yDir.y) * 2.0f;
        float invSqrtResult = 1.0f / sqrtResult;
        out->w = (xDir.y - yDir.x) * invSqrtResult;
        out->x = (zDir.x + xDir.z) * invSqrtResult;
        out->y = (zDir.y + yDir.z) * invSqrtResult;
        out->z = 0.25 * sqrtResult;
    }
}

void quatLerp(struct Quaternion* a, struct Quaternion* b, float t, struct Quaternion* out) {
    float tInv = 1.0f - t;

    if (quatDot(a, b) < 0) {
        out->x = tInv * a->x - t * b->x;
        out->y = tInv * a->y - t * b->y;
        out->z = tInv * a->z - t * b->z;
        out->w = tInv * a->w - t * b->w;
    } else {
        out->x = tInv * a->x + t * b->x;
        out->y = tInv * a->y + t * b->y;
        out->z = tInv * a->z + t * b->z;
        out->w = tInv * a->w + t * b->w;
    }

    quatNormalize(out, out);
}

void quatApplyAngularVelocity(struct Quaternion* input, struct Vector3* w, float timeStep, struct Quaternion* output) {
    struct Quaternion velocityAsQuat;
    velocityAsQuat.w = 0.0f;
    velocityAsQuat.x = w->x * timeStep * 0.5f;
    velocityAsQuat.y = w->y * timeStep * 0.5f;
    velocityAsQuat.z = w->z * timeStep * 0.5f;

    struct Quaternion intermediate;
    quatMultiply(&velocityAsQuat, input, &intermediate);

    quatAdd(&intermediate, input, output);
    quatNormalize(output, output);
}


void quatDecompose(struct Quaternion* input, struct Vector3* axis, float* angle) {
    float axisMag = sqrtf(input->x * input->x + input->y * input->y + input->z * input->z);

    if (axisMag < 0.0001f) {
        *axis = gUp;
        *angle = 0.0f;
        return;
    }

    float magInv = 1.0f / axisMag;

    axis->x = input->x * magInv;
    axis->y = input->y * magInv;
    axis->z = input->z * magInv;
    *angle = sinf(axisMag) * 2.0f;
}

float quatDot(struct Quaternion* a, struct Quaternion* b) {
    return a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w;
}