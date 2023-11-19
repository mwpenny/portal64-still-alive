#include "ZSorter.h"

#include <vector>

struct SingleFace {
    aiFace* face;
    std::pair<Bone*, Bone*> bonePair;
    ExtendedMesh* mesh;
    Material* material;
    aiNode* meshRoot;
    ai_real sortKey;
};

struct FaceIndices {
    unsigned indices[3];
};

struct EditableMesh {
    std::vector<unsigned> sourceIndex;
    std::vector<FaceIndices> faces;
};

void renderChunksFlush(EditableMesh& target, std::vector<SingleFace>::iterator start, std::vector<SingleFace>::iterator end, std::map<unsigned, unsigned>& indexMapping) {
    for (auto face = start; face != end; ++face) {
        FaceIndices newFace;
        
        for (unsigned i = 0; i < 3 && i < face->face->mNumIndices; ++i) {
            auto index = indexMapping.find(face->face->mIndices[i]);

            if (index == indexMapping.end()) {
                newFace.indices[i] = 0;
            } else {
                newFace.indices[i] = index->second;
            }
        }

        target.faces.push_back(newFace);
    }

    indexMapping.clear();
}

aiMatrix4x4 aiBuildOffsetMatrix(aiNode* node) {
    if (!node) {
        return aiMatrix4x4();
    }

    return aiBuildOffsetMatrix(node->mParent) * node->mTransformation;
}

RenderChunk renderChunksRebuildFromFaces(const aiScene* scene, std::vector<SingleFace>::iterator start, std::vector<SingleFace>::iterator end, unsigned chunkIndex, DisplayListSettings& settings, BoneHierarchy& boneHeirarchy) {
    std::map<unsigned, unsigned> indexMapping;

    EditableMesh mesh;

    std::vector<SingleFace>::iterator lastFlushStart = start;

    ExtendedMesh* source = lastFlushStart->mesh;

    for (auto face = start; face != end; ++face) {
        unsigned neededIndices = 0;

        for (unsigned i = 0; i < face->face->mNumIndices; ++i) {
            if (indexMapping.find(face->face->mIndices[i]) == indexMapping.end()) {
                ++neededIndices;
            }
        }

        if (neededIndices + indexMapping.size() > (unsigned)settings.mVertexCacheSize) {
            renderChunksFlush(mesh, lastFlushStart, face, indexMapping);
        }

        for (unsigned i = 0; i < face->face->mNumIndices; ++i) {
            if (indexMapping.find(face->face->mIndices[i]) != indexMapping.end()) {
                continue;
            }

            unsigned index = face->face->mIndices[i];
            unsigned newIndex = mesh.sourceIndex.size();
            indexMapping[index] = newIndex;
            mesh.sourceIndex.push_back(index);
        }
    }

    renderChunksFlush(mesh, lastFlushStart, end, indexMapping);

    aiMesh* newAiMesh = new aiMesh();
    newAiMesh->mNumVertices = mesh.sourceIndex.size();

    newAiMesh->mVertices = new aiVector3D[newAiMesh->mNumVertices];

    if (source->mMesh->mNormals) {
        newAiMesh->mNormals = new aiVector3D[newAiMesh->mNumVertices];
    }

    for (int i = 0; i < 8; ++i) {
        if (source->mMesh->mTextureCoords[i]) {
            newAiMesh->mTextureCoords[i] = new aiVector3D[newAiMesh->mNumVertices];
        }
        if (source->mMesh->mColors[i]) {
            newAiMesh->mColors[i] = new aiColor4D[newAiMesh->mNumVertices];
        }
    }

    newAiMesh->mAABB = source->mMesh->mAABB;
    newAiMesh->mName = source->mMesh->mName;
    newAiMesh->mMaterialIndex = source->mMesh->mMaterialIndex;

    char indexAsString[10];
    sprintf(indexAsString, "_%d", chunkIndex);
    newAiMesh->mName.Append(indexAsString);

    for (unsigned newIndex = 0; newIndex < newAiMesh->mNumVertices; ++newIndex) {
        unsigned sourceIndex = mesh.sourceIndex[newIndex];
        newAiMesh->mVertices[newIndex] = source->mMesh->mVertices[sourceIndex];
        if (newAiMesh->mNormals) newAiMesh->mNormals[newIndex] = source->mMesh->mNormals[sourceIndex];

        for (int i = 0; i < 8; ++i) {
            if (newAiMesh->mTextureCoords[i]) newAiMesh->mTextureCoords[i][newIndex] = source->mMesh->mTextureCoords[i][sourceIndex];
            if (newAiMesh->mColors[i]) newAiMesh->mColors[i][newIndex] = source->mMesh->mColors[i][sourceIndex];
        }
    }

    newAiMesh->mNumFaces = mesh.faces.size();
    newAiMesh->mFaces = new aiFace[newAiMesh->mNumFaces];

    newAiMesh->mNumBones = 0;

    newAiMesh->mNumBones = source->mMesh->mNumBones;

    if (newAiMesh->mNumBones) {
        newAiMesh->mBones = new aiBone*[newAiMesh->mNumBones];

        for (unsigned i = 0; i < newAiMesh->mNumBones; ++i) {
            aiBone* bone = new aiBone;
            aiBone* srcBone = source->mMesh->mBones[i];

            bone->mArmature = srcBone->mArmature;
            bone->mName = srcBone->mName;
            bone->mNode = srcBone->mNode;
            bone->mOffsetMatrix = srcBone->mOffsetMatrix;

            std::vector<unsigned> attachedIndices;

            std::set<unsigned> includedBefore;
            for (unsigned weightIndex = 0; weightIndex < srcBone->mNumWeights; ++weightIndex) {
                includedBefore.insert(srcBone->mWeights[weightIndex].mVertexId);
            }

            for (unsigned vertexIndex = 0; vertexIndex < newAiMesh->mNumVertices; ++vertexIndex) {
                unsigned sourceIndex = mesh.sourceIndex[vertexIndex];

                if (includedBefore.find(sourceIndex) != includedBefore.end()) {
                    attachedIndices.push_back(vertexIndex);
                }
            }

            bone->mNumWeights = attachedIndices.size();
            bone->mWeights = new aiVertexWeight[attachedIndices.size()];

            for (unsigned weightIndex = 0; weightIndex < attachedIndices.size(); ++weightIndex) {
                bone->mWeights[weightIndex].mVertexId = attachedIndices[weightIndex];
                bone->mWeights[weightIndex].mWeight = 1.0f;
            }

            newAiMesh->mBones[i] = bone;
        }
    }

    for (unsigned faceIndex = 0; faceIndex < newAiMesh->mNumFaces; ++faceIndex) {
        newAiMesh->mFaces[faceIndex].mNumIndices = 3;
        newAiMesh->mFaces[faceIndex].mIndices = new unsigned[3];
        std::copy(mesh.faces[faceIndex].indices, mesh.faces[faceIndex].indices + 3, newAiMesh->mFaces[faceIndex].mIndices);
    }

    std::shared_ptr<ExtendedMesh> newMesh(new ExtendedMesh(newAiMesh, boneHeirarchy));

    return RenderChunk(start->bonePair, newMesh, start->meshRoot, start->material);
}

int renderChunkSortGroup(const std::string& nodeName) {
    std::size_t startSortGroup = nodeName.find("sort-group ");

    if (startSortGroup == std::string::npos) {
        return 0;
    }

    startSortGroup += strlen("sort-group ");

    std::size_t endSortGroup = nodeName.find(" ", startSortGroup);

    std::string value = nodeName.substr(startSortGroup, endSortGroup == std::string::npos ? std::string::npos : endSortGroup - startSortGroup);

    return std::stoi(value);
}

std::vector<RenderChunk> renderChunksSortByZ(const aiScene* scene, const std::vector<RenderChunk>& source, DisplayListSettings& settings, BoneHierarchy& boneHeirarchy) {
    std::vector<SingleFace> faces;

    for (auto chunk : source) {
        int sortGroup = renderChunkSortGroup(chunk.mMeshRoot->mName.C_Str());

        for (auto face : chunk.GetFaces()) {
            SingleFace singleFace;

            aiVector3D faceAverage;

            for (unsigned i = 0; i < face->mNumIndices; ++i) {
                faceAverage = faceAverage + chunk.mMesh->mMesh->mVertices[face->mIndices[i]];
            }

            faceAverage = (1.0f / (ai_real)face->mNumIndices) * faceAverage;

            singleFace.face = face;
            singleFace.mesh = chunk.mMesh.get();
            singleFace.material = chunk.mMaterial;
            singleFace.meshRoot = chunk.mMeshRoot;
            singleFace.sortKey = faceAverage * settings.mSortDirection + sortGroup * 10000.0f;
            singleFace.bonePair = chunk.mBonePair;

            faces.push_back(singleFace);
        }
    }

    std::sort(faces.begin(), faces.end(), [](const SingleFace& a, const SingleFace& b) {
        return a.sortKey < b.sortKey;
    });

    std::vector<RenderChunk> result;

    auto lastStart = faces.begin();

    int chunkIndex = 0;

    for (auto it = faces.begin(); it != faces.end(); ++it) {
        if (lastStart->mesh != it->mesh || lastStart->material != it->material || lastStart->meshRoot != it->meshRoot || lastStart->bonePair != it->bonePair) {
            result.push_back(renderChunksRebuildFromFaces(scene, lastStart, it, chunkIndex, settings, boneHeirarchy));
            lastStart = it;
            ++chunkIndex;
        }
    }

    result.push_back(renderChunksRebuildFromFaces(scene, lastStart, faces.end(), chunkIndex, settings, boneHeirarchy));

    return result;
} 