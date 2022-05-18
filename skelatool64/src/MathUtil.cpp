#include "MathUtl.h"

#include <math.h>

aiVector3D min(const aiVector3D& a, const aiVector3D& b) {
    return aiVector3D(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
}

aiVector3D max(const aiVector3D& a, const aiVector3D& b) {
    return aiVector3D(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
}