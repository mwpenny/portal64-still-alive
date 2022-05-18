#ifndef _BONE_HIERARCHY_H
#define _BONE_HIERARCHY_H

#include <assimp/vector3.h>
#include <assimp/quaternion.h>
#include <assimp/scene.h>

#include <vector>
#include <memory>
#include <string>
#include <set>
#include <map>
#include <ostream>

#include "ErrorCode.h"
#include "./definitions/DataChunk.h"

class CFileDefinition;

class Bone {
public:
    Bone(int index, std::string name, Bone* parent, const aiVector3D& restPosition, const aiQuaternion& restRotation, const aiVector3D& restScale);

    int GetIndex();
    const std::string& GetName();
    Bone* GetParent();

    std::unique_ptr<DataChunk> GenerateRestPosiitonData(float scale, aiQuaternion rotation);

    static Bone* FindCommonAncestor(Bone* a, Bone* b);
    /**
     *  Assumes ancestor is an ancestor of decendant
     *  Returns the child bone of ancestor that is an ancestor
     *  of decendant or is decendant
     */
    static Bone* StepDownTowards(Bone* ancestor, Bone* decendant);

    static bool CompareBones(Bone* a, Bone* b);
    // return -1 if a is null
    static int GetBoneIndex(Bone* a);
private:
    int mIndex;
    std::string mName;
    Bone* mParent;
    aiVector3D mRestPosition;
    aiQuaternion mRestRotation;
    aiQuaternion mRestScale;

    std::vector<Bone*> mChildren;
};

class BoneHierarchy {
public:
    void SearchForBones(aiNode* node, Bone* currentBoneParent, std::set<std::string>& knownBones, bool parentIsBone);
    void SearchForBonesInScene(const aiScene* scene);
    Bone* BoneByIndex(unsigned index);
    Bone* BoneForName(std::string name);
    bool HasData() const;
    unsigned int GetBoneCount() const;

    void GenerateRestPosiitonData(CFileDefinition& fileDef, const std::string& variableName, float scale, aiQuaternion rotation);
private:
    std::vector<std::unique_ptr<Bone>> mBones;
    std::map<std::string, Bone*> mBoneByName;
};

#endif