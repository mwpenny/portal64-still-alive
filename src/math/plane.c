#include "plane.h"
#include "mathf.h"

int planeRayIntersection(struct Plane* plane, struct Vector3* rayOrigin, struct Vector3* rayDirection, float* rayDistance) {
    float normalDot = vector3Dot(&plane->normal, rayDirection);

    if (fabsf(normalDot) < 0.00001f) {
        return 0;
    }

    *rayDistance = -(vector3Dot(rayOrigin, &plane->normal) + plane->d) / normalDot;

    return 1;
}