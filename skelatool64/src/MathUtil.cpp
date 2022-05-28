#include "MathUtl.h"

#include <math.h>

aiVector3D min(const aiVector3D& a, const aiVector3D& b) {
    return aiVector3D(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
}

aiVector3D max(const aiVector3D& a, const aiVector3D& b) {
    return aiVector3D(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
}

float distanceToAABB(const aiAABB& aabb, const aiVector3D& point) {
    aiVector3D closestPoint = max(aabb.mMin, min(aabb.mMax, point));

    if (closestPoint == point) {
        // return negative penetration depth
        aiVector3D maxOffset = point - aabb.mMax;
        aiVector3D minOffset = aabb.mMin - point;

        return std::max(
            std::max(
                std::max(maxOffset.x, maxOffset.y),
                std::max(maxOffset.z, minOffset.x)
            ),
            std::max(minOffset.y, minOffset.z)
        );
    } else {
        return (closestPoint - point).Length();
    }
}

bool doesAABBOverlap(const aiAABB& a, const aiAABB& b) {
    return a.mMin.x < b.mMax.x && b.mMin.x < a.mMax.x &&
        a.mMin.y < b.mMax.y && b.mMin.y < a.mMax.y &&
        a.mMin.z < b.mMax.z && b.mMin.z < a.mMax.z;
}