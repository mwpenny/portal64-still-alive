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

float calculateLerp(struct Vector3* a, struct Vector3* b, struct Vector3* point) {
    struct Vector3 v0;
    vector3Sub(b, a, &v0);

    float denom = vector3MagSqrd(&v0);

    if (denom < 0.00000001f) {
        return 0.5f;
    }

    struct Vector3 pointOffset;
    vector3Sub(point, a, &pointOffset);

    return vector3Dot(&pointOffset, &v0) / denom;
}

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

    float denom = d00 * d11 - d01 * d01;

    if (fabsf(denom) < 0.000001f) {
        if (d00 > d11) {
            // b is other point
            output->y = calculateLerp(a, b, point);
            output->x = 1.0 - output->y;
            output->z = 0.0f;

        } else {
            // c is other point
            output->z = calculateLerp(a, c, point);
            output->x = 1.0f - output->z;
            output->z = 0.0f;
        }
        return;
    }

    float denomInv = 1.0f / (d00 * d11 - d01 * d01);
    output->y = (d11 * d20 - d01 * d21) * denomInv;
    output->z = (d00 * d21 - d01 * d20) * denomInv;
    output->x = 1.0f - output->y - output->z;
}

void evaluateBarycentricCoords(struct Vector3* a, struct Vector3* b, struct Vector3* c, struct Vector3* bary, struct Vector3* output) {
    vector3Scale(a, output, bary->x);
    vector3AddScaled(output, b, bary->y, output);
    vector3AddScaled(output, c, bary->z, output);
}