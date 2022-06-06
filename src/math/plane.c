#include "plane.h"
#include "mathf.h"

void planeInitWithNormalAndPoint(struct Plane* plane, struct Vector3* normal, struct Vector3* point) {
    plane->normal = *normal;
    plane->d = -vector3Dot(normal, point);
}

int planeRayIntersection(struct Plane* plane, struct Vector3* rayOrigin, struct Vector3* rayDirection, float* rayDistance) {
    float normalDot = vector3Dot(&plane->normal, rayDirection);

    if (fabsf(normalDot) < 0.00001f) {
        return 0;
    }

    *rayDistance = -(vector3Dot(rayOrigin, &plane->normal) + plane->d) / normalDot;

    return 1;
}


float planePointDistance(struct Plane* plane, struct Vector3* point) {
    return vector3Dot(&plane->normal, point) + plane->d;
}

void planeProjectPoint(struct Plane* plane, struct Vector3* point, struct Vector3* output) {
    float distance = planePointDistance(plane, point);
    vector3AddScaled(point, &plane->normal, distance, output);
}

// TODO figure out what to do when two points are the same
void calculateBarycentricCoords(struct Vector3* a, struct Vector3* b, struct Vector3* c, struct Vector3* point, struct Vector3* output) {
    struct Vector3 v0;
    struct Vector3 v1;
    struct Vector3 v2;

    vector3Sub(b, a, &v0);
    vector3Sub(c, a, &v1);
    vector3Sub(point, a, &v2);

    float d00 = vector3Dot(&v0, &v0);
    float d01 = vector3Dot(&v0, &v1);
    float d11 = vector3Dot(&v1, &v1);
    float d20 = vector3Dot(&v2, &v0);
    float d21 = vector3Dot(&v2, &v1);

    float denom = 1.0f / (d00 * d11 - d01 * d01);
    output->y = (d11 * d20 - d01 * d21) * denom;
    output->z = (d00 * d21 - d01 * d20) * denom;
    output->x = 1.0f - output->y - output->z;
}

void evaluateBarycentricCoords(struct Vector3* a, struct Vector3* b, struct Vector3* c, struct Vector3* bary, struct Vector3* output) {
    vector3Scale(a, output, bary->x);
    vector3AddScaled(output, b, bary->y, output);
    vector3AddScaled(output, c, bary->z, output);
}