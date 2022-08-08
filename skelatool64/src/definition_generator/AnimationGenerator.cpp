#include "AnimationGenerator.h"

#include "./DefinitionGenerator.h"
#include "../AnimationTranslator.h"
#include <set>
#include <string>
#include <map>

std::shared_ptr<NodeAnimationInfo> findNodesForWithAnimation(const aiScene* scene, const std::vector<aiNode*>& usedNodes, float modelScale) {
    std::set<std::string> animatedNodeNames;

    for (unsigned animIndex = 0; animIndex < scene->mNumAnimations; ++animIndex) {
        aiAnimation* animation = scene->mAnimations[animIndex];

        for (unsigned channelIndex = 0; channelIndex < animation->mNumChannels; ++channelIndex) {
            animatedNodeNames.insert(animation->mChannels[channelIndex]->mNodeName.C_Str());
        }
    }

    std::set<aiMesh*> usedMeshes;

    for (auto node : usedNodes) {
        for (unsigned i = 0; i < node->mNumMeshes; ++i) {
            usedMeshes.insert(scene->mMeshes[node->mMeshes[i]]);
        }
    }

    for (auto mesh : usedMeshes) {
        for (unsigned boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
            animatedNodeNames.insert(mesh->mBones[boneIndex]->mName.C_Str());
        }
    }

    std::map<const aiNode*, int> nodeOrder;
    std::set<aiNode*> nodesWithAnimationData;

    forEachNode(scene->mRootNode, [&](aiNode* node) -> void {
        if (animatedNodeNames.find(node->mName.C_Str()) != animatedNodeNames.end()) {
            nodesWithAnimationData.insert(node);
        }

        nodeOrder.insert(std::make_pair(node, nodeOrder.size()));
    });

    std::shared_ptr<NodeAnimationInfo> result(new NodeAnimationInfo());

    for (auto node : nodesWithAnimationData) {
        std::unique_ptr<AnimationNodeInfo> nodeInfo(new AnimationNodeInfo());

        aiNode* currentNode = node;

        while (currentNode->mParent && nodesWithAnimationData.find(currentNode->mParent) == nodesWithAnimationData.end()) {
            currentNode = currentNode->mParent;

            nodeInfo->relativeTransform = currentNode->mTransformation * nodeInfo->relativeTransform;
        }

        if (!currentNode->mParent) {
            nodeInfo->relativeTransform = aiMatrix4x4(aiVector3D(1, 1, 1) * modelScale, aiQuaternion(), aiVector3D()) * nodeInfo->relativeTransform;
        }

        nodeInfo->node = node;
        nodeInfo->parent = currentNode->mParent;

        result->nodesWithAnimation.push_back(std::move(nodeInfo));
    }

    std::sort(result->nodesWithAnimation.begin(), result->nodesWithAnimation.end(), [&](const std::unique_ptr<AnimationNodeInfo>& a, const std::unique_ptr<AnimationNodeInfo>& b) -> bool {
        return nodeOrder[a->node] < nodeOrder[b->node];
    });

    return result;
}

std::vector<SKAnimationHeader> generateAnimationData(const aiScene* scene, BoneHierarchy& bones, CFileDefinition& fileDef, float fixedPointScale, float modelScale, const aiQuaternion& rotation, unsigned short targetTicksPerSecond) {
    std::vector<SKAnimationHeader> animations;

    for (unsigned i = 0; i < scene->mNumAnimations; ++i) {
        SKAnimation animation;
        if (translateAnimationToSK(*scene->mAnimations[i], animation, bones, fixedPointScale, modelScale, rotation, targetTicksPerSecond)) {
            std::string animationName = fileDef.GetUniqueName(scene->mAnimations[i]->mName.C_Str());
            unsigned short firstChunkSize = formatAnimationChunks(animationName, animation.chunks, fileDef);

            SKAnimationHeader header;
            header.firstChunkSize = firstChunkSize;
            header.ticksPerSecond = targetTicksPerSecond;
            header.maxTicks = animation.maxTicks;
            header.animationName = animationName;

            animations.push_back(header);
        }
    }

    return animations;
}

void generateAnimationForScene(const aiScene* scene, CFileDefinition &fileDefinition, DisplayListSettings& settings) {
    BoneHierarchy& bones = fileDefinition.GetBoneHierarchy();

    std::string bonesName = fileDefinition.GetUniqueName("default_bones");
    std::string boneParentName = fileDefinition.GetUniqueName("bone_parent");
    bones.GenerateRestPosiitonData(fileDefinition, bonesName);
    std::string boneCountName = bonesName + "_COUNT";
    std::transform(boneCountName.begin(), boneCountName.end(), boneCountName.begin(), ::toupper);
    fileDefinition.AddMacro(boneCountName, std::to_string(bones.GetBoneCount()));

    aiMatrix4x4 baseTransform(
        aiVector3D(settings.mModelScale, settings.mModelScale, settings.mModelScale), 
        settings.mRotateModel, 
        aiVector3D(0, 0, 0)
    );

    std::string animationsName = fileDefinition.GetUniqueName("animations");
    auto animations = generateAnimationData(scene, bones, fileDefinition, settings.mFixedPointScale, settings.mModelScale, settings.mRotateModel, settings.mTicksPerSecond);

    std::unique_ptr<StructureDataChunk> animationNameData(new StructureDataChunk());

    int index = 0;
    for (auto it = animations.begin(); it != animations.end(); ++it) {
        std::unique_ptr<StructureDataChunk> animationChunk(new StructureDataChunk());

        animationChunk->AddPrimitive(it->firstChunkSize);
        animationChunk->AddPrimitive(it->ticksPerSecond);
        animationChunk->AddPrimitive(it->maxTicks);
        animationChunk->AddPrimitive(0);
        animationChunk->AddPrimitive(std::string("(struct SKAnimationChunk*)") + it->animationName);
        animationChunk->AddPrimitive(0);

        animationNameData->Add(std::move(animationChunk));

        std::string animationIndex = fileDefinition.GetUniqueName(it->animationName + "_INDEX");
        std::transform(animationIndex.begin(), animationIndex.end(), animationIndex.begin(), ::toupper);
        fileDefinition.AddMacro(animationIndex, std::to_string(index));

        ++index;
    }
    std::unique_ptr<DataFileDefinition> headerDef(new DataFileDefinition("struct SKAnimationHeader", animationsName, true, "_geo", std::move(animationNameData)));
    headerDef->AddTypeHeader("\"sk64/skelatool_clip.h\"");
    fileDefinition.AddDefinition(std::move(headerDef));

    std::unique_ptr<StructureDataChunk> boneParentDataChunk(new StructureDataChunk());

    for (unsigned int boneIndex = 0; boneIndex < bones.GetBoneCount(); ++boneIndex) {
        Bone* bone = bones.BoneByIndex(boneIndex);
        if (bone->GetParent()) {
            boneParentDataChunk->AddPrimitive(bone->GetParent()->GetIndex());
        } else {
            boneParentDataChunk->AddPrimitive(0xFFFF);
        }
    }

    fileDefinition.AddDefinition(std::unique_ptr<FileDefinition>(new DataFileDefinition("unsigned short", boneParentName, true, "_geo", std::move(boneParentDataChunk))));
}