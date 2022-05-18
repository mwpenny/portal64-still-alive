
#include "ExtendedMesh.h"

#include <algorithm>
#include "MathUtl.h"

aiMesh* copyMesh(aiMesh* mesh) {
    aiMesh* result = new aiMesh();
    result->mNumVertices = mesh->mNumVertices;

    result->mVertices = new aiVector3D[result->mNumVertices];
    std::copy(mesh->mVertices, mesh->mVertices + result->mNumVertices, result->mVertices);

    if (mesh->mNormals) {
        result->mNormals = new aiVector3D[result->mNumVertices];
        std::copy(mesh->mNormals, mesh->mNormals + result->mNumVertices, result->mNormals);
    }

    result->mMaterialIndex = mesh->mMaterialIndex;

    result->mNumFaces = mesh->mNumFaces;
    result->mFaces = new aiFace[mesh->mNumFaces];
    std::copy(mesh->mFaces, mesh->mFaces + result->mNumFaces, result->mFaces);

    for (int i = 0; i < 8; ++i) {
        if (mesh->mTextureCoords[i]) {
            result->mTextureCoords[i] = new aiVector3D[result->mNumVertices];
            result->mNumUVComponents[i] = mesh->mNumUVComponents[i];

            std::copy(mesh->mTextureCoords[i], mesh->mTextureCoords[i] + result->mNumVertices, result->mTextureCoords[i]);
        }
    }

    return result;
}


ExtendedMesh::ExtendedMesh(const ExtendedMesh& other):
    mMesh(copyMesh(other.mMesh)),
    mPointInverseTransform(other.mPointInverseTransform),
    mNormalInverseTransform(other.mNormalInverseTransform),
    mVertexBones(other.mVertexBones),
    mFacesForBone(other.mFacesForBone),
    bbMin(other.bbMin),
    bbMax(other.bbMax) {
    for (auto& it : mFacesForBone) {
        std::vector<aiFace*> faces;

        for (auto face : it.second) {
            faces.push_back(mMesh->mFaces + (face - other.mMesh->mFaces));
        }
        
        mFacesForBone[it.first] = faces;
    }

    for (auto& it : mBoneSpanningFaces) {
        std::vector<aiFace*> faces;

        for (auto face : it.second) {
            faces.push_back(mMesh->mFaces + (face - other.mMesh->mFaces));
        }
        
        mBoneSpanningFaces[it.first] = faces;
    }
}

ExtendedMesh::ExtendedMesh(aiMesh* mesh, BoneHierarchy& boneHierarchy) :
    mMesh(mesh) {
    mVertexBones.resize(mMesh->mNumVertices);
    mPointInverseTransform.resize(mMesh->mNumVertices);
    mNormalInverseTransform.resize(mMesh->mNumVertices);

    std::set<Bone*> bonesAsSet;

    for (unsigned int boneIndex = 0; boneIndex < mMesh->mNumBones; ++boneIndex) {
        aiBone* bone = mMesh->mBones[boneIndex];
        Bone* hierarchyBone = boneHierarchy.BoneForName(bone->mName.C_Str());
        bonesAsSet.insert(hierarchyBone);

        aiMatrix3x3 normalTransform(bone->mOffsetMatrix);
        normalTransform = normalTransform.Transpose().Inverse();

        for (unsigned int vertexIndex = 0; vertexIndex < bone->mNumWeights; ++vertexIndex) {
            unsigned int vertexId = bone->mWeights[vertexIndex].mVertexId;
            mVertexBones[vertexId] = hierarchyBone;
            mPointInverseTransform[vertexId] = &bone->mOffsetMatrix;
            mNormalInverseTransform[vertexId] = new aiMatrix3x3(normalTransform);
        }
    }

    PopulateFacesForBone();
    RecalcBB();
}

ExtendedMesh::~ExtendedMesh() {
    for (unsigned int i = 0; i < mNormalInverseTransform.size(); ++i) {
        if (mNormalInverseTransform[i]) {
            delete mNormalInverseTransform[i];
        }
    }
}

void ExtendedMesh::RecalcBB() {
    bbMin = mMesh->mVertices[0];
    bbMax = mMesh->mVertices[0];

    for (unsigned i = 1; i < mMesh->mNumVertices; ++i) {
        bbMin = min(bbMin, mMesh->mVertices[i]);
        bbMax = max(bbMax, mMesh->mVertices[i]);
    }
}

bool ExtendedMesh::isFaceOneBone(aiFace* face) {
    Bone* bone = mVertexBones[face->mIndices[0]];

    for (unsigned int i = 1; i < face->mNumIndices; ++i) {
        if (mVertexBones[face->mIndices[i]] != bone) {
            return false;
        }
    }

    return true;
}

std::pair<Bone*, Bone*> ExtendedMesh::findTransitionPairForFace(aiFace* face) {
    Bone* ancestor = mVertexBones[face->mIndices[0]];

    for (unsigned int i = 1; i < face->mNumIndices; ++i) {
        ancestor = Bone::FindCommonAncestor(ancestor, mVertexBones[face->mIndices[i]]);
    }

    Bone* second = ancestor;

    for (unsigned int i = 0; i < face->mNumIndices; ++i) {
        if (mVertexBones[face->mIndices[i]] != ancestor) {
            second = Bone::StepDownTowards(ancestor, mVertexBones[face->mIndices[i]]);
            break;
        }
    }

    return std::make_pair(ancestor, second);
}

void ExtendedMesh::PopulateFacesForBone() {
    for (unsigned int faceIndex = 0; faceIndex < mMesh->mNumFaces; ++faceIndex) {
        aiFace* face = &mMesh->mFaces[faceIndex];
        if (isFaceOneBone(face)) {
            mFacesForBone[mVertexBones[face->mIndices[0]]].push_back(face);
        } else {
            mBoneSpanningFaces[findTransitionPairForFace(face)].push_back(face);
        }
    }
}

std::shared_ptr<ExtendedMesh> ExtendedMesh::Transform(const aiMatrix4x4& transform) const {
    std::shared_ptr<ExtendedMesh> result(new ExtendedMesh(*this));

    aiMatrix3x3 rotationOnly(transform);
    for (unsigned i = 0; i < result->mMesh->mNumVertices; ++i) {
        result->mMesh->mVertices[i] = transform * result->mMesh->mVertices[i];

        if (result->mMesh->mNormals) {
            result->mMesh->mNormals[i] = rotationOnly * result->mMesh->mNormals[i];
            result->mMesh->mNormals[i].NormalizeSafe();
        }
    }
    result->RecalcBB();

    return result;
}

void ExtendedMesh::ReplaceColor(const aiColor4D& color) {
    if (mMesh->mColors[0]) {
        delete [] mMesh->mColors[0];
    }

    mMesh->mColors[0] = new aiColor4D[mMesh->mNumVertices];

    for (unsigned i = 0; i < mMesh->mNumVertices; ++i) {
        mMesh->mColors[0][i] = color;
    }
}

void getMeshFaceGroups(aiMesh* mesh, std::vector<std::shared_ptr<std::set<aiFace*>>>& result) {
    result.clear();

    std::map<int, int> indexToGroup;

    for (unsigned faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
        aiFace* face = &mesh->mFaces[faceIndex];

        int currentGroup = -1;

        for (unsigned indexIndex = 0; indexIndex < face->mNumIndices; ++indexIndex) {
            unsigned index = face->mIndices[indexIndex];

            auto indexToGroupFind = indexToGroup.find(index);

            int indexGroup = indexToGroupFind == indexToGroup.end() ? -1 : indexToGroupFind->second;

            if (indexGroup == -1) {
                if (currentGroup == -1) {
                    indexGroup = result.size();
                    currentGroup = indexGroup;

                    result.push_back(std::shared_ptr<std::set<aiFace*>>(new std::set<aiFace*>()));
                } else {
                    indexGroup = currentGroup;
                }

                indexToGroup[index] = indexGroup;
            }

            if (currentGroup == -1) {
                currentGroup = indexGroup;
            }

            auto currentGroupSet = result[currentGroup];
            auto indexGroupSet = result[indexGroup];

            if (currentGroupSet != indexGroupSet) {
                // merge both the groups
                for (auto face : *indexGroupSet) {
                    currentGroupSet->insert(face);
                }
                // have both group numbers point to the same group
                result[indexGroup] = currentGroupSet;
            }

            // add current face to group
            currentGroupSet->insert(face);
        }
    }

    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
}

void cubeProjectSingleFace(aiMesh* mesh, std::set<aiFace*>& faces, double sTile, double tTile) {
    aiVector3D normal;

    for (auto face : faces) {
        for (unsigned i = 0; i < face->mNumIndices; ++i) {
            normal += mesh->mNormals[face->mIndices[i]];
        }
    }

    normal.NormalizeSafe();

    aiVector3D left;
    aiVector3D up;
    float minLeft = 10000000000.0f;
    float minUp = 10000000000.0f;

    if (fabs(normal.y) > 0.7) {
        up = aiVector3D(0.0f, 0.0f, 1.0f);
        left = aiVector3D(1.0f, 0.0f, 0.0f);
    } else if (fabs(normal.z) > 0.7) {
        up = aiVector3D(0.0f, 1.0f, 0.0f);
        left = aiVector3D(1.0f, 0.0f, 0.0f);
    } else {
        up = aiVector3D(0.0f, 1.0f, 0.0f);
        left = aiVector3D(0.0f, 0.0f, 1.0f);
    }

    for (auto face : faces) {
        for (unsigned i = 0; i < face->mNumIndices; ++i) {
            aiVector3D vertex = mesh->mVertices[face->mIndices[i]];

            minLeft = std::min(minLeft, vertex * left);
            minUp = std::min(minUp, vertex * up);
        }
    }

    for (auto face : faces) {
        for (unsigned i = 0; i < face->mNumIndices; ++i) {
            int index = face->mIndices[i];
            aiVector3D vertex = mesh->mVertices[index];

            float sCoord = vertex * left - minLeft;
            float tCoord = vertex * up - minUp;

            mesh->mTextureCoords[0][index] = aiVector3D(sCoord * sTile, tCoord * tTile, 0.0f);
        }
    }
}

void ExtendedMesh::CubeProjectTex(double sTile, double tTile) {
    if (!mMesh->mTextureCoords[0]) {
        mMesh->mNumUVComponents[0] = 2;
        mMesh->mTextureCoords[0] = new aiVector3D[mMesh->mNumVertices];
    }

    std::vector<std::shared_ptr<std::set<aiFace*>>> faceGroups;
    getMeshFaceGroups(mMesh, faceGroups);

    for (auto group : faceGroups) {
        cubeProjectSingleFace(mMesh, *group.get(), sTile, tTile);
    }
}

void findAdjacentVertices(aiMesh* mesh, unsigned fromIndex, std::set<int>& result) {
    for (unsigned faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
        aiFace* face = &mesh->mFaces[faceIndex];

        for (unsigned index = 0; index < face->mNumIndices; ++index) {
            if (face->mIndices[index] == fromIndex) {
                result.insert(face->mIndices[(index + 1) % face->mNumIndices]);
                result.insert(face->mIndices[(index + face->mNumIndices - 1) % face->mNumIndices]);
                break;
            }
        }
    }
}

std::string ExtendedMesh::GetMaterialName(aiMaterial* material) {
    aiString name;
    material->Get(AI_MATKEY_NAME, name);
    return name.C_Str();
}