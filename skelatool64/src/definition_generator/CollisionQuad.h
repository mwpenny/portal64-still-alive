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

    std::unique_ptr<DataChunk> Generate();

    void ToLocalCoords(const aiVector3D& input, short& outX, short& outY);

    bool IsCoplanar(ExtendedMesh& mesh, float relativeScale) const;

    aiAABB BoundingBox() const;
};

#endif