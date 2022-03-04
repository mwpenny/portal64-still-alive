#ifndef __MATH_PLANE_H__
#define __MATH_PLANE_H__

#include "vector3.h"

struct Plane {
    struct Vector3 normal;
    float d;
};

int planeRayIntersection(struct Plane* plane, struct Vector3* rayOrigin, struct Vector3* rayDirection, float* rayDistance);

float planePointDistance(struct Plane* plane, struct Vector3* point);
void planeProjectPoint(struct Plane* plane, struct Vector3* point, struct Vector3* output);

#endif