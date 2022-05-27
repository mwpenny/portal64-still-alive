#ifndef __LINE_H__
#define __LINE_H__

#include "../math/vector3.h"

int lineNearestApproach(struct Vector3* aAt, struct Vector3* aDir, struct Vector3* bAt, struct Vector3* bDir, float* aOut, float* bOut);

#endif