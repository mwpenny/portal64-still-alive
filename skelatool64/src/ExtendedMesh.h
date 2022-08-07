#ifndef _EXTENDED_MESH_H
#define _EXTENDED_MESH_H

#include <assimp/mesh.h>
#include "BoneHierarchy.h"
#include <vector>
#include <map>
#include <set>

enum class VertexType {
    PosUVNormal,
    PosUVColor,
    POSUVTangent,
    POSUVMinusTangent,
    POSUVCotangent,
    POSUVMinusCotangent,
};

class ExtendedMesh {
public:
    ExtendedMesh(const ExtendedMesh& other);
    ExtendedMesh(aiMesh* mesh, BoneHierarchy& boneHierarchy);
    ~ExtendedMesh();
    aiMesh* mMesh;
    std::vector<aiMatrix4x4> mPointInverseTransform;
    std::vector<aiMatrix3x3> mNormalInverseTransform;
    std::vector<Bone*> mVertexBones;
    std::map<Bone*, std::vector<aiFace*>> mFacesForBone;
    // first bone in pair is always the parent of the second
    std::map<std::pair<Bone*, Bone*>, std::vector<aiFace*>> mBoneSpanningFaces;
    aiVector3D bbMin;
    aiVector3D bbMax;

    void RecalcBB();

    std::shared_ptr<ExtendedMesh> Transform(const aiMatrix4x4& transform) const;
    void ReplaceColor(const aiColor4D& color);
    void CubeProjectTex(double sTile, double tTile);

    bool isFaceOneBone(aiFace* face);
    std::pair<Bone*, Bone*> findTransitionPairForFace(aiFace* face);

    static std::string GetMaterialName(aiMaterial* material, const std::string& forceMaterial);
private:
    void PopulateFacesForBone();
};

aiMesh* copyMesh(aiMesh* mesh);

void findAdjacentVertices(aiMesh* mesh, unsigned fromIndex, std::set<int>& result);

#endif