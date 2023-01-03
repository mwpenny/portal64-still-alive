
#ifndef _VECTOR3_H
#define _VECTOR3_H

struct Vector3i32 {
    int x, y, z;
};

struct Vector3 {
    float x, y, z;
};

struct Vector3u8 {
    char x, y, z;
};

extern struct Vector3 gRight;
extern struct Vector3 gUp;
extern struct Vector3 gForward;
extern struct Vector3 gZeroVec;
extern struct Vector3 gOneVec;

#define VECTOR3_AS_ARRAY(vector) ((float*)(vector))

void vector3Abs(struct Vector3* in, struct Vector3* out);
void vector3Negate(struct Vector3* in, struct Vector3* out);
void vector3Scale(struct Vector3* in, struct Vector3* out, float scale);
void vector3Add(struct Vector3* a, struct Vector3* b, struct Vector3* out);
void vector3AddScaled(struct Vector3* a, struct Vector3* normal, float scale, struct Vector3* out);
void vector3Sub(struct Vector3* a, struct Vector3* b, struct Vector3* out);
void vector3Multiply(struct Vector3* a, struct Vector3* b, struct Vector3* out);
void vector3Normalize(struct Vector3* in, struct Vector3* out);
void vector3Lerp(struct Vector3* a, struct Vector3* b, float t, struct Vector3* out);
float vector3Dot(struct Vector3* a, struct Vector3* b);
float vector3MagSqrd(struct Vector3* a);
float vector3DistSqrd(struct Vector3* a, struct Vector3* b);
void vector3Cross(struct Vector3* a, struct Vector3* b, struct Vector3* out);
void vector3Perp(struct Vector3* a, struct Vector3* out);
void vector3Project(struct Vector3* in, struct Vector3* normal, struct Vector3* out);
void vector3ProjectPlane(struct Vector3* in, struct Vector3* normal, struct Vector3* out);
int vector3MoveTowards(struct Vector3* from, struct Vector3* towards, float maxDistance, struct Vector3* out);
void vector3TripleProduct(struct Vector3* a, struct Vector3* b, struct Vector3* c, struct Vector3* output);

void vector3Max(struct Vector3* a, struct Vector3* b, struct Vector3* out);
void vector3Min(struct Vector3* a, struct Vector3* b, struct Vector3* out);

int vector3IsZero(struct Vector3* vector);

void vector3ToVector3u8(struct Vector3* input, struct Vector3u8* output);

float vector3EvalBarycentric1D(struct Vector3* baryCoords, float a, float b, float c);

#endif