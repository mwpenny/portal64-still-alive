
#include "vector3.h"
#include "mathf.h"

#include <ultra64.h>

struct Vector3 gRight = {1.0f, 0.0f, 0.0f};
struct Vector3 gUp = {0.0f, 1.0f, 0.0f};
struct Vector3 gForward = {0.0f, 0.0f, 1.0f};
struct Vector3 gZeroVec = {0.0f, 0.0f, 0.0f};
struct Vector3 gOneVec = {1.0f, 1.0f, 1.0f};

void vector3Abs(struct Vector3* in, struct Vector3* out) {
    out->x = fabsf(in->x);
    out->y = fabsf(in->y);
    out->z = fabsf(in->z);
}

void vector3Negate(struct Vector3* in, struct Vector3* out) {
    out->x = -in->x;
    out->y = -in->y;
    out->z = -in->z;
}

void vector3Scale(struct Vector3* in, struct Vector3* out, float scale) {
    out->x = in->x * scale;
    out->y = in->y * scale;
    out->z = in->z * scale;
}

void vector3Add(struct Vector3* a, struct Vector3* b, struct Vector3* out) {
    out->x = a->x + b->x;
    out->y = a->y + b->y;
    out->z = a->z + b->z;   
}

void vector3AddScaled(struct Vector3* a, struct Vector3* normal, float scale, struct Vector3* out) {
    out->x = a->x + normal->x * scale;
    out->y = a->y + normal->y * scale;
    out->z = a->z + normal->z * scale;
}

void vector3Sub(struct Vector3* a, struct Vector3* b, struct Vector3* out) {
    out->x = a->x - b->x;
    out->y = a->y - b->y;
    out->z = a->z - b->z;
}

void vector3Multiply(struct Vector3* a, struct Vector3* b, struct Vector3* out) {
    out->x = a->x * b->x;
    out->y = a->y * b->y;
    out->z = a->z * b->z;
}

void vector3Normalize(struct Vector3* in, struct Vector3* out) {
    float denom = in->x * in->x + in->y * in->y + in->z * in->z;

    if (denom == 0.0f) {
        out->x = 0.0f;
        out->y = 0.0f;
        out->z = 0.0f;
    } else {
        float invSqrt = 1.0f / sqrtf(denom);
        vector3Scale(in, out, invSqrt);
    }
}

void vector3Lerp(struct Vector3* a, struct Vector3* b, float t, struct Vector3* out) {
    float tFlip = 1.0f - t;
    out->x = a->x * tFlip + b->x * t;
    out->y = a->y * tFlip + b->y * t;
    out->z = a->z * tFlip + b->z * t;
}

float vector3Dot(struct Vector3* a, struct Vector3* b) {
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

float vector3MagSqrd(struct Vector3* a) {
    return a->x * a->x + a->y * a->y + a->z * a->z;
}

float vector3DistSqrd(struct Vector3* a, struct Vector3* b) {
    float x = a->x - b->x;
    float y = a->y - b->y;
    float z = a->z - b->z;

    return x * x + y * y + z * z;
}

void vector3Cross(struct Vector3* a, struct Vector3* b, struct Vector3* out) {
    out->x = a->y * b->z - a->z * b->y;
    out->y = a->z * b->x - a->x * b->z;
    out->z = a->x * b->y - a->y * b->x;
}

void vector3Perp(struct Vector3* a, struct Vector3* out) {
    if (fabsf(a->x) > fabsf(a->z)) {
        vector3Cross(a, &gForward, out);
    } else {
        vector3Cross(a, &gRight, out);
    }
}

void vector3Project(struct Vector3* in, struct Vector3* normal, struct Vector3* out) {
    float mag = vector3Dot(in, normal);
    out->x = normal->x * mag;
    out->y = normal->y * mag;
    out->z = normal->z * mag;
}

void vector3ProjectPlane(struct Vector3* in, struct Vector3* normal, struct Vector3* out) {
    float mag = vector3Dot(in, normal);
    out->x = in->x - normal->x * mag;
    out->y = in->y - normal->y * mag;
    out->z = in->z - normal->z * mag;
}

int vector3MoveTowards(struct Vector3* from, struct Vector3* towards, float maxDistance, struct Vector3* out) {
    float distance = vector3DistSqrd(from, towards);

    if (distance < maxDistance * maxDistance) {
        *out = *towards;
        return 1;
    } else {
        float scale = maxDistance / sqrtf(distance);
        out->x = (towards->x - from->x) * scale + from->x;
        out->y = (towards->y - from->y) * scale + from->y;
        out->z = (towards->z - from->z) * scale + from->z;
        return 0;
    }
}

void vector3TripleProduct(struct Vector3* a, struct Vector3* b, struct Vector3* c, struct Vector3* output) {
    vector3Scale(b, output, vector3Dot(a, c));
    vector3AddScaled(output, a, -vector3Dot(b, c), output);
}

void vector3Max(struct Vector3* a, struct Vector3* b, struct Vector3* out) {
    out->x = MAX(a->x, b->x);
    out->y = MAX(a->y, b->y);
    out->z = MAX(a->z, b->z);
}

void vector3Min(struct Vector3* a, struct Vector3* b, struct Vector3* out) {
    out->x = MIN(a->x, b->x);
    out->y = MIN(a->y, b->y);
    out->z = MIN(a->z, b->z);
}

int vector3IsZero(struct Vector3* vector) {
    return vector->x == 0.0f && vector->y == 0.0f && vector->z == 0.0f;
}

void vector3ToVector3u8(struct Vector3* input, struct Vector3u8* output) {
    output->x = floatTos8norm(input->x);
    output->y = floatTos8norm(input->y);
    output->z = floatTos8norm(input->z);
}

float vector3EvalBarycentric1D(struct Vector3* baryCoords, float a, float b, float c) {
    return baryCoords->x * a + baryCoords->y * b + baryCoords->z * c;
}