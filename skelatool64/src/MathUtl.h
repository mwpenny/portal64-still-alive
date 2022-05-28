#ifndef MATH_UTIL_H
#define MATH_UTIL_H

#include <assimp/mesh.h>

aiVector3D min(const aiVector3D& a, const aiVector3D& b);
aiVector3D max(const aiVector3D& a, const aiVector3D& b);

float distanceToAABB(const aiAABB& aabb, const aiVector3D& point);
bool doesAABBOverlap(const aiAABB& a, const aiAABB& b);

#endif