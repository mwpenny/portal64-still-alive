#ifndef _RENDER_CHUNK_H
#define _RENDER_CHUNK_H

#include <assimp/scene.h>

#include "ExtendedMesh.h"
#include "BoneHierarchy.h"
#include "materials/Material.h"

#include <vector>
#include <memory>
#include <map>

class RenderChunk {
public:
    RenderChunk(std::pair<Bone*, Bone*> bonePair, std::shared_ptr<ExtendedMesh> mesh, Material* material);
    // if bones are the same, chunk cooresponds to a single bone
    // the bones can be null
    std::pair<Bone*, Bone*> mBonePair;
    std::shared_ptr<ExtendedMesh> mMesh;
    Material* mMaterial;

    VertexType GetVertexType();
    int GetTextureWidth();
    int GetTextureHeight();

    const std::vector<aiFace*>& GetFaces();
private:
};

void extractChunks(const aiScene* scene, std::vector<std::shared_ptr<ExtendedMesh>>& meshes, std::vector<RenderChunk>& result, std::map<std::string, std::shared_ptr<Material>>& mMaterials);

void orderChunks(std::vector<RenderChunk>& result);

#endif