#include "line.h"

int lineNearestApproach(struct Vector3* aAt, struct Vector3* aDir, struct Vector3* bAt, struct Vector3* bDir, float* aOut, float* bOut) {
    float edgesDot = vector3Dot(aDir, bDir);

    float denomInv = 1.0f - edgesDot * edgesDot;

    if (denomInv < 0.0001f) {
        return 0;
    }

    denomInv = 1.0f / denomInv;

    struct Vector3 offset;
    vector3Sub(bAt, aAt, &offset);

    float cubeDot = vector3Dot(bDir, &offset);
    float edgeDot = vector3Dot(aDir, &offset);

    *aOut = (edgeDot - edgesDot * cubeDot) * denomInv;
    *bOut = (edgesDot * edgeDot - cubeDot) * denomInv;

    return 1;
}