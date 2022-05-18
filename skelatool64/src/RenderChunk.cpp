
#include "RenderChunk.h"
#include <algorithm>

RenderChunk::RenderChunk(std::pair<Bone*, Bone*> bonePair, std::shared_ptr<ExtendedMesh> mesh, Material* material): 
    mBonePair(bonePair),
    mMesh(mesh),
    mMaterial(material) {

}

VertexType RenderChunk::GetVertexType() {
    return Material::GetVertexType(mMaterial);
}

int RenderChunk::GetTextureWidth() {
    return Material::TextureWidth(mMaterial);
}

int RenderChunk::GetTextureHeight() {
    return Material::TextureHeight(mMaterial);
}

const std::vector<aiFace*>& RenderChunk::GetFaces() {
    if (mBonePair.first == mBonePair.second) {
        auto result = mMesh->mFacesForBone.find(mBonePair.first);
        return result->second;
    } else {
        auto result = mMesh->mBoneSpanningFaces.find(mBonePair);
        return result->second;
    }
}

void extractChunks(const aiScene* scene, std::vector<std::shared_ptr<ExtendedMesh>>& meshes, std::vector<RenderChunk>& result, std::map<std::string, std::shared_ptr<Material>>& materials) {
    for (auto it = meshes.begin(); it != meshes.end(); ++it) {
        Material* materialPtr = NULL;

        auto material = materials.find(ExtendedMesh::GetMaterialName(scene->mMaterials[(*it)->mMesh->mMaterialIndex]));

        if (material != materials.end()) {
            materialPtr = material->second.get();
        }

        for (auto boneSegment = (*it)->mFacesForBone.begin(); boneSegment != (*it)->mFacesForBone.end(); ++boneSegment) {
            result.push_back(RenderChunk(
                std::make_pair(boneSegment->first, boneSegment->first),
                *it,
                materialPtr
            ));
        }

        for (auto pairSegment = (*it)->mBoneSpanningFaces.begin(); pairSegment != (*it)->mBoneSpanningFaces.end(); ++pairSegment) {
            result.push_back(RenderChunk(pairSegment->first, *it, materialPtr));
        }
    }
}

void orderChunks(std::vector<RenderChunk>& result) {
    // TODO solve the traveling salesman algorithm
    std::sort(result.begin(), result.end(), 
        [](const RenderChunk& a, const RenderChunk& b) -> bool {
            int aSecondScore = Bone::GetBoneIndex(a.mBonePair.second);
            int bSecondScore = Bone::GetBoneIndex(b.mBonePair.second);

            if (aSecondScore == bSecondScore) {
                return Bone::GetBoneIndex(a.mBonePair.first) < Bone::GetBoneIndex(b.mBonePair.first);
            }
            
            return aSecondScore < bSecondScore;
    });
}