
#include "./SceneModification.h"

#include <map>
#include <set>
#include <memory>
#include "./BoneHierarchy.h"
#include "./ExtendedMesh.h"

void generateVertexMapping(aiMesh* mesh, std::vector<aiFace*> faces, std::map<unsigned int, unsigned int>& result) {
    std::set<unsigned int> usedIndices;

    for (auto faceIt = faces.begin(); faceIt != faces.end(); ++faceIt) {
        aiFace* face = *faceIt;

        for (unsigned int index = 0; index < face->mNumIndices; ++index) {
            usedIndices.insert(face->mIndices[index]);
        }
    }

    unsigned int mappedIndex = 0;

    for (unsigned int vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
        if (usedIndices.find(vertexIndex) != usedIndices.end()) {
            result[vertexIndex] = mappedIndex;
            ++mappedIndex;
        }
    }
}

void filterOutBones(aiMesh* source, aiMesh* target, std::map<unsigned int, unsigned int>& vertexMapping) {
    target->mNumBones = 0;
    target->mBones = new aiBone*[source->mNumBones];

    for (unsigned int sourceBoneIndex = 0; sourceBoneIndex < source->mNumBones; ++sourceBoneIndex) {
        aiBone* newBone = new aiBone();

        aiBone* sourceBone = source->mBones[sourceBoneIndex];

        newBone->mNumWeights = 0;
        newBone->mName = sourceBone->mName;
        newBone->mOffsetMatrix = sourceBone->mOffsetMatrix;
        newBone->mWeights = new aiVertexWeight[sourceBone->mNumWeights];

        unsigned int numWeights = 0;

        for (unsigned int boneVertIndex = 0; boneVertIndex < sourceBone->mNumWeights; ++boneVertIndex) {
            auto newIndexIt = vertexMapping.find(sourceBone->mWeights[boneVertIndex].mVertexId);

            if (newIndexIt != vertexMapping.end()) {
                newBone->mWeights[numWeights].mVertexId = newIndexIt->second;
                newBone->mWeights[numWeights].mWeight = sourceBone->mWeights[boneVertIndex].mWeight;
                ++numWeights;
            }
        }

        if (numWeights) {
            newBone->mNumWeights = numWeights;
            target->mBones[target->mNumBones] = newBone;
            ++target->mNumBones;
        } else {
            delete newBone;
        }
    }

    if (target->mNumBones == 0) {
        delete [] target->mBones;
        target->mBones = nullptr;
    }
}

void filterOutFaces(aiMesh* source, aiMesh* target, std::map<unsigned int, unsigned int>& vertexMapping, std::vector<aiFace*> faces) {
    target->mNumFaces = faces.size();
    target->mFaces = new aiFace[faces.size()];

    for (unsigned int currentFace = 0; currentFace < faces.size(); ++currentFace) {
        aiFace newFace;

        newFace.mNumIndices = faces[currentFace]->mNumIndices;
        newFace.mIndices = new unsigned int[newFace.mNumIndices];

        for (unsigned int index = 0; index < newFace.mNumIndices; ++index) {
            newFace.mIndices[index] = vertexMapping[faces[currentFace]->mIndices[index]];
        }

        target->mFaces[currentFace] = newFace;
    }
}

aiMesh* subMesh(aiMesh* mesh, std::vector<aiFace*> faces) {
    aiMesh* result = new aiMesh();

    std::map<unsigned int, unsigned int> vertexMapping;
    generateVertexMapping(mesh, faces, vertexMapping);

    result->mNumVertices = vertexMapping.size();
    result->mVertices = new aiVector3D[result->mNumVertices];
    result->mMaterialIndex = mesh->mMaterialIndex;
    result->mMethod = mesh->mMethod;
    result->mName = mesh->mName;
    if (mesh->mNormals) result->mNormals = new aiVector3D[result->mNumVertices];
    if (mesh->mTextureCoords[0]) result->mTextureCoords[0] = new aiVector3D[result->mNumVertices];
    if (mesh->mColors[0]) result->mColors[0] = new aiColor4D[result->mNumVertices];

    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        auto newIndexIt = vertexMapping.find(i);

        if (newIndexIt != vertexMapping.end()) {
            unsigned int newIndex = newIndexIt->second;

            result->mVertices[newIndex] = mesh->mVertices[i];
            if (result->mNormals) result->mNormals[newIndex] = mesh->mNormals[i];
            if (result->mTextureCoords[0]) result->mTextureCoords[0][newIndex] = mesh->mTextureCoords[0][i];
            if (result->mColors[0]) result->mColors[0][newIndex] = mesh->mColors[0][i];
        }
    }

    filterOutBones(mesh, result, vertexMapping);
    filterOutFaces(mesh, result, vertexMapping, faces);

    return result;
}

void splitSceneByBones(aiScene* targetScene) {
    std::vector<aiMesh*> newMeshes;
    
    BoneHierarchy bones;

    bones.SearchForBonesInScene(targetScene);

    for (unsigned int i = 0; i < targetScene->mNumMeshes; ++i) {
        aiMesh* currMesh = targetScene->mMeshes[i];
        std::unique_ptr<ExtendedMesh> extendedMesh(new ExtendedMesh(currMesh, bones));
        
        for (auto newFaces = extendedMesh->mFacesForBone.begin(); newFaces != extendedMesh->mFacesForBone.end(); ++newFaces) {
            newMeshes.push_back(subMesh(currMesh, newFaces->second));
        }

        for (auto newFaces = extendedMesh->mBoneSpanningFaces.begin(); newFaces != extendedMesh->mBoneSpanningFaces.end(); ++newFaces) {
            newMeshes.push_back(subMesh(currMesh, newFaces->second));
        }

        delete currMesh;
    }
    
    delete [] targetScene->mMeshes;

    targetScene->mNumMeshes = newMeshes.size();
    targetScene->mMeshes = new aiMesh*[newMeshes.size()];
    std::copy(newMeshes.begin(), newMeshes.end(), targetScene->mMeshes);
}