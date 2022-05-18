#ifndef __MES_H__
#define __MES_H__

#include <assimp/vector3.h>
#include <assimp/mesh.h>
#include <vector>

struct Sphere {
    Sphere();
    Sphere(const aiVector3D& center, float radius);

    aiVector3D center;
    float radius;

    bool Contains(const aiVector3D& point);
};

Sphere minimumEnclosingSphere(const std::vector<aiVector3D>& input);
Sphere minimumEnclosingSphereForMeshes(const std::vector<aiMesh*>& input);

#endif