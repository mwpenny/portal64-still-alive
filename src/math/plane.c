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