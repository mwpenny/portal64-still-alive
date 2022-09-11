
#include <iostream>
#include "BoneHierarchy.h"
#include "CFileDefinition.h"

Bone::Bone(int index, std::string name, Bone* parent, const aiVector3D& restPosition, const aiQuaternion& restRotation, const aiVector3D& restScale):
    mIndex(index),
    mName(name),
    mParent(parent),
    mRestPosition(restPosition),
    mRestRotation(restRotation),
    mRestScale(restScale) {

    if (mParent) {
        mParent->mChildren.push_back(this);
    }
}

int Bone::GetIndex() {
    return mIndex;
}

const std::string& Bone::GetName() {
    return mName;
}

Bone* Bone::GetParent() {
    return mParent;
}

std::unique_ptr<DataChunk> Bone::GenerateRestPosiitonData() {
    std::unique_ptr<StructureDataChunk> result(new StructureDataChunk());

    result->Add(std::unique_ptr<DataChunk>(new StructureDataChunk(mRestPosition)));
    result->Add(std::unique_ptr<DataChunk>(new StructureDataChunk(mRestRotation)));
    result->Add(std::unique_ptr<DataChunk>(new StructureDataChunk(mRestScale)));

    return std::move(result);
}

Bone* Bone::FindCommonAncestor(Bone* a, Bone* b) {
    std::set<Bone*> hierarchyDifference;

    Bone* curr = a;

    while (curr) {
        hierarchyDifference.insert(curr);
        curr = curr->mParent;
    }

    curr = b;
    
    while (curr) {
        hierarchyDifference.erase(curr);
        curr = curr->mParent;
    }

    curr = a;

    while (hierarchyDifference.find(curr) != hierarchyDifference.end()) {
        curr = curr->mParent;
    }

    return curr;
}

Bone* Bone::StepDownTowards(Bone* ancestor, Bone* decendant) {
    Bone* curr = decendant;

    while (curr && curr->mParent != ancestor) {
        curr = curr->mParent;
    }

    return curr;
}

bool Bone::CompareBones(Bone* a, Bone* b) {
    if (a == nullptr && b == nullptr) {
        return false;
    } else if (a == nullptr) {
        return true;
    } else if (b == nullptr) {
        return false;
    } else {
        return a->mIndex < b->mIndex;
    }
}

int Bone::GetBoneIndex(Bone* a) {
    if (a == nullptr) {
        return -1;
    } else {
        return a->mIndex;
    }
}

void BoneHierarchy::PopulateWithAnimationNodeInfo(const NodeAnimationInfo& animInfo, float fixedPointScale, aiQuaternion& rotation) {
    aiMatrix4x4 rotationMatrix(rotation.GetMatrix());

    for (auto& node : animInfo.nodesWithAnimation) {
        Bone* parent = nullptr;

        if (node->parent) {
            auto parentFind = mBoneByName.find(node->parent->mName.C_Str());

            if (parentFind != mBoneByName.end()) {
                parent = parentFind->second;
            }
        }

        std::string boneName = node->node->mName.C_Str();

        aiVector3D restPosition;
        aiQuaternion restRotation;
        aiVector3D restScale;

        aiMatrix4x4 fullRestTransform = node->relativeTransform * node->node->mTransformation;

        if (parent == nullptr) {
            fullRestTransform = rotationMatrix * fullRestTransform;
        }

        fullRestTransform.Decompose(restScale, restRotation, restPosition);

        mBones.push_back(std::unique_ptr<Bone>(new Bone(
            mBones.size(),
            boneName,
            parent,
            restPosition * fixedPointScale,
            restRotation,
            restScale
        )));

        mBoneByName.insert(std::pair<std::string, Bone*>(boneName, mBones.back().get()));
    }
}

void BoneHierarchy::SearchForBones(aiNode* node, Bone* currentBoneParent, std::set<std::string>& knownBones, bool parentIsBone, float fixedPointScale) {
    if (knownBones.find(node->mName.C_Str()) != knownBones.end()) {
        aiVector3D restPosition;
        aiQuaternion restRotation;
        aiVector3D restScale;
        node->mTransformation.Decompose(restScale, restRotation, restPosition);

        mBones.push_back(std::unique_ptr<Bone>(new Bone(
            mBones.size(),
            node->mName.C_Str(),
            currentBoneParent,
            restPosition * fixedPointScale,
            restRotation,
            restScale
        )));

        currentBoneParent = mBones[mBones.size() - 1].get();
        mBoneByName.insert(std::pair<std::string, Bone*>(node->mName.C_Str(), currentBoneParent));

        parentIsBone = true;
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        SearchForBones(node->mChildren[i], currentBoneParent, knownBones, parentIsBone, fixedPointScale);
    }
}

void BoneHierarchy::SearchForBonesInScene(const aiScene* scene, float fixedPointScale) {
    std::set<std::string> knownBones;

    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
        aiMesh* mesh = scene->mMeshes[meshIndex];

        for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
            knownBones.insert(mesh->mBones[boneIndex]->mName.C_Str());
        }
    }

    SearchForBones(scene->mRootNode, nullptr, knownBones, false, fixedPointScale);
}

Bone* BoneHierarchy::BoneByIndex(unsigned index) {
    return mBones[index].get();
}

Bone* BoneHierarchy::BoneForName(std::string name) {
    auto result = mBoneByName.find(name);

    if (result == mBoneByName.end()) {
        return nullptr;
    } else {
        return result->second;
    }
}

void BoneHierarchy::GenerateRestPosiitonData(CFileDefinition& fileDef, const std::string& variableName) {
    if (mBones.size() == 0) return;

    std::unique_ptr<StructureDataChunk> transformData(new StructureDataChunk());

    for (unsigned int boneIndex = 0; boneIndex < mBones.size(); ++boneIndex) {
        transformData->Add(std::move(mBones[boneIndex]->GenerateRestPosiitonData()));

        std::string boneName = fileDef.GetUniqueName(mBones[boneIndex]->GetName() + "_BONE");
        std::transform(boneName.begin(), boneName.end(), boneName.begin(), ::toupper);

        fileDef.AddMacro(boneName, std::to_string(boneIndex));
    }

    std::unique_ptr<FileDefinition> restPosDef(new DataFileDefinition("struct Transform", variableName, true, "_geo", std::move(transformData)));
    restPosDef->AddTypeHeader("\"math/transform.h\"");
    fileDef.AddDefinition(std::move(restPosDef));
}

bool BoneHierarchy::HasData() const {
    return mBones.size() > 0;
}

unsigned int BoneHierarchy::GetBoneCount() const {
    return mBones.size();
}