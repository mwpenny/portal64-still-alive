#ifndef __COLLISION_QUAD_H__
#define __COLLISION_QUAD_H__

#include <assimp/mesh.h>
#include "../ExtendedMesh.h"

struct CollisionQuad {
    CollisionQuad(aiMesh* mesh, const aiMatrix4x4& transform);

    aiVector3D corner;
    aiVector3D edgeA;
    float edgeALength;
    aiVector3D edgeB;
    float edgeBLength;
    aiVector3D normal;
    float thickness;

    std::unique_ptr<DataChunk> Generate() const;

    bool IsCoplanar(ExtendedMesh& mesh, float relativeScale) const;
    bool IsCoplanar(const aiVector3D& input) const;

    aiAABB BoundingBox() const;
};

#endif